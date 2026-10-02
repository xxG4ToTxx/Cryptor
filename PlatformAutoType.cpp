#include "Platform.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <dlfcn.h>
#include <poll.h>
#endif

namespace {

void ValidatePassword(const unsigned char* password, std::size_t length)
{
    if (password == nullptr || length < 2) {
        throw std::invalid_argument(
            "A password must contain at least two characters"
        );
    }

    for (std::size_t index = 0; index < length; ++index) {
        if (password[index] < 0x20 || password[index] > 0x7e) {
            throw std::invalid_argument(
                "Auto-type supports printable ASCII passwords only"
            );
        }
    }
}


void WaitForTargetWindow()
{
    std::cout << "Switch to the target field in";
    for (int seconds = 3; seconds > 0; --seconds) {
        std::cout << ' ' << seconds << std::flush;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << "...\n";
}


#ifdef _WIN32

bool ClearWindowsClipboard()
{
    for (int attempt = 0; attempt < 20; ++attempt) {
        if (OpenClipboard(nullptr)) {
            const bool cleared = EmptyClipboard() != 0;
            CloseClipboard();
            return cleared;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    return false;
}


void SetWindowsClipboard(const unsigned char* password, std::size_t length)
{
    const std::size_t firstHalfLength = length / 2;
    HGLOBAL clipboardMemory = GlobalAlloc(
        GMEM_MOVEABLE,
        firstHalfLength + 1
    );
    if (clipboardMemory == nullptr) {
        throw std::runtime_error("Could not allocate clipboard memory");
    }

    auto* clipboardText =
        static_cast<unsigned char*>(GlobalLock(clipboardMemory));
    if (clipboardText == nullptr) {
        GlobalFree(clipboardMemory);
        throw std::runtime_error("Could not lock clipboard memory");
    }

    std::copy_n(password, firstHalfLength, clipboardText);
    clipboardText[firstHalfLength] = '\0';
    GlobalUnlock(clipboardMemory);

    if (!OpenClipboard(nullptr)) {
        GlobalFree(clipboardMemory);
        throw std::runtime_error("Could not open the clipboard");
    }

    if (!EmptyClipboard()) {
        CloseClipboard();
        GlobalFree(clipboardMemory);
        throw std::runtime_error("Could not clear the clipboard");
    }

    if (SetClipboardData(CF_TEXT, clipboardMemory) == nullptr) {
        CloseClipboard();
        GlobalFree(clipboardMemory);
        throw std::runtime_error("Could not place the password on the clipboard");
    }

    CloseClipboard();
}


void PasteWindowsClipboard()
{
    INPUT events[4]{};
    events[0].type = INPUT_KEYBOARD;
    events[0].ki.wVk = VK_CONTROL;
    events[1].type = INPUT_KEYBOARD;
    events[1].ki.wVk = 'V';
    events[2].type = INPUT_KEYBOARD;
    events[2].ki.wVk = 'V';
    events[2].ki.dwFlags = KEYEVENTF_KEYUP;
    events[3].type = INPUT_KEYBOARD;
    events[3].ki.wVk = VK_CONTROL;
    events[3].ki.dwFlags = KEYEVENTF_KEYUP;

    if (SendInput(4, events, sizeof(INPUT)) != 4) {
        INPUT releases[2]{};
        releases[0].type = INPUT_KEYBOARD;
        releases[0].ki.wVk = 'V';
        releases[0].ki.dwFlags = KEYEVENTF_KEYUP;
        releases[1].type = INPUT_KEYBOARD;
        releases[1].ki.wVk = VK_CONTROL;
        releases[1].ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(2, releases, sizeof(INPUT));
        throw std::runtime_error("Could not paste the clipboard password half");
    }
}


void TypeWindowsPassword(
    const unsigned char* password,
    std::size_t firstIndex,
    std::size_t length
)
{
    for (std::size_t index = firstIndex; index < length; ++index) {
        INPUT events[2]{};
        events[0].type = INPUT_KEYBOARD;
        events[0].ki.wScan = password[index];
        events[0].ki.dwFlags = KEYEVENTF_UNICODE;
        events[1] = events[0];
        events[1].ki.dwFlags |= KEYEVENTF_KEYUP;

        if (SendInput(2, events, sizeof(INPUT)) != 2) {
            events[0].ki.dwFlags |= KEYEVENTF_KEYUP;
            SendInput(1, events, sizeof(INPUT));
            throw std::runtime_error("Could not type the remaining password");
        }
    }
}

#elif defined(__linux__)

using XTestQueryExtensionFunction =
    Bool (*)(Display*, int*, int*, int*, int*);
using XTestFakeKeyEventFunction =
    Bool (*)(Display*, unsigned int, Bool, unsigned long);

struct XTestApi {
    void* library = nullptr;
    XTestQueryExtensionFunction queryExtension = nullptr;
    XTestFakeKeyEventFunction fakeKeyEvent = nullptr;

    ~XTestApi()
    {
        if (library != nullptr) {
            dlclose(library);
        }
    }
};


void LoadXTest(XTestApi& api)
{
    api.library = dlopen("libXtst.so.6", RTLD_NOW | RTLD_LOCAL);
    if (api.library == nullptr) {
        throw std::runtime_error("The X11 XTest runtime library is unavailable");
    }

    api.queryExtension = reinterpret_cast<XTestQueryExtensionFunction>(
        dlsym(api.library, "XTestQueryExtension")
    );
    api.fakeKeyEvent = reinterpret_cast<XTestFakeKeyEventFunction>(
        dlsym(api.library, "XTestFakeKeyEvent")
    );
    if (api.queryExtension == nullptr || api.fakeKeyEvent == nullptr) {
        throw std::runtime_error("The X11 XTest functions are unavailable");
    }
}


struct KeyStroke {
    KeyCode code;
    bool shift;
};


KeyStroke FindKeyStroke(Display* display, unsigned char character)
{
    int firstCode = 0;
    int lastCode = 0;
    XDisplayKeycodes(display, &firstCode, &lastCode);

    for (int code = firstCode; code <= lastCode; ++code) {
        for (int level = 0; level <= 1; ++level) {
            if (XkbKeycodeToKeysym(display, code, 0, level) == character) {
                return {static_cast<KeyCode>(code), level == 1};
            }
        }
    }

    throw std::runtime_error(
        "The active keyboard layout cannot type this password"
    );
}


std::vector<KeyStroke> PrepareX11KeyStrokes(
    Display* display,
    const unsigned char* password,
    std::size_t firstIndex,
    std::size_t length,
    KeyCode& shift
)
{
    std::vector<KeyStroke> keys;
    keys.reserve(length - firstIndex);

    for (std::size_t index = firstIndex; index < length; ++index) {
        keys.push_back(FindKeyStroke(display, password[index]));
    }

    shift = XKeysymToKeycode(display, XK_Shift_L);
    for (const KeyStroke& key : keys) {
        if (key.shift && shift == 0) {
            throw std::runtime_error(
                "The active keyboard layout cannot type this password"
            );
        }
    }

    return keys;
}


void SendX11Key(
    Display* display,
    const XTestApi& test,
    KeyCode code,
    bool pressed
)
{
    if (code == 0 ||
        !test.fakeKeyEvent(display, code, pressed, CurrentTime))
    {
        throw std::runtime_error("Could not send a simulated key event");
    }
    XFlush(display);
}


void PasteX11Clipboard(
    Display* display,
    const XTestApi& test,
    KeyCode control,
    KeyCode v
)
{
    SendX11Key(display, test, control, true);
    try {
        SendX11Key(display, test, v, true);
        SendX11Key(display, test, v, false);
        SendX11Key(display, test, control, false);
    }
    catch (...) {
        test.fakeKeyEvent(display, v, False, CurrentTime);
        test.fakeKeyEvent(display, control, False, CurrentTime);
        XFlush(display);
        throw;
    }
}


bool RespondToSelectionRequest(
    Display* display,
    const XSelectionRequestEvent& request,
    Atom targets,
    Atom utf8String,
    const unsigned char* password,
    std::size_t firstHalfLength
)
{
    const Atom property =
        request.property == None ? request.target : request.property;
    bool dataProvided = false;

    if (request.target == targets) {
        const Atom supportedTargets[] = {targets, utf8String, XA_STRING};
        XChangeProperty(
            display,
            request.requestor,
            property,
            XA_ATOM,
            32,
            PropModeReplace,
            reinterpret_cast<const unsigned char*>(supportedTargets),
            3
        );
    }
    else if (request.target == utf8String || request.target == XA_STRING) {
        XChangeProperty(
            display,
            request.requestor,
            property,
            request.target,
            8,
            PropModeReplace,
            password,
            static_cast<int>(firstHalfLength)
        );
        dataProvided = true;
    }

    XSelectionEvent response{};
    response.type = SelectionNotify;
    response.display = display;
    response.requestor = request.requestor;
    response.selection = request.selection;
    response.target = request.target;
    response.property =
        request.target == targets || dataProvided ? property : None;
    response.time = request.time;
    XSendEvent(
        display,
        request.requestor,
        False,
        0,
        reinterpret_cast<XEvent*>(&response)
    );
    XFlush(display);
    return dataProvided;
}


bool WaitForX11ClipboardRequest(
    Display* display,
    Atom targets,
    Atom utf8String,
    const unsigned char* password,
    std::size_t firstHalfLength
)
{
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::seconds(2);

    while (std::chrono::steady_clock::now() < deadline) {
        const auto remaining = std::chrono::duration_cast<
            std::chrono::milliseconds
        >(deadline - std::chrono::steady_clock::now()).count();
        pollfd connection{ConnectionNumber(display), POLLIN, 0};
        const int ready = poll(
            &connection,
            1,
            static_cast<int>(remaining)
        );
        if (ready < 0) {
            throw std::runtime_error("Could not wait for a clipboard request");
        }
        if (ready == 0) {
            break;
        }

        while (XPending(display) != 0) {
            XEvent event{};
            XNextEvent(display, &event);
            if (event.type == SelectionRequest &&
                RespondToSelectionRequest(
                    display,
                    event.xselectionrequest,
                    targets,
                    utf8String,
                    password,
                    firstHalfLength
                ))
            {
                return true;
            }
        }
    }

    return false;
}


void TypeX11Password(
    Display* display,
    const XTestApi& test,
    const std::vector<KeyStroke>& keys,
    KeyCode shift
)
{
    for (const KeyStroke& key : keys) {
        if (key.shift) {
            SendX11Key(display, test, shift, true);
        }

        try {
            SendX11Key(display, test, key.code, true);
            SendX11Key(display, test, key.code, false);
        }
        catch (...) {
            test.fakeKeyEvent(display, key.code, False, CurrentTime);
            if (key.shift) {
                test.fakeKeyEvent(display, shift, False, CurrentTime);
                XFlush(display);
            }
            throw;
        }

        if (key.shift) {
            SendX11Key(display, test, shift, false);
        }
    }
}


bool ClearX11Clipboard(Display* display, Window owner, Atom clipboard)
{
    if (XGetSelectionOwner(display, clipboard) != owner) {
        return true;
    }

    XSetSelectionOwner(display, clipboard, None, CurrentTime);
    XSync(display, False);
    return XGetSelectionOwner(display, clipboard) != owner;
}

#endif

}


void Platform::AutoTypePassword(
    const unsigned char* password,
    std::size_t length
)
{
    ValidatePassword(password, length);
    const std::size_t firstHalfLength = length / 2;

#ifdef _WIN32
    WaitForTargetWindow();
    SetWindowsClipboard(password, length);
    try {
        PasteWindowsClipboard();
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        TypeWindowsPassword(password, firstHalfLength, length);
    }
    catch (...) {
        if (!ClearWindowsClipboard()) {
            throw std::runtime_error(
                "Auto-type failed and the clipboard could not be cleared"
            );
        }
        throw;
    }

    if (!ClearWindowsClipboard()) {
        throw std::runtime_error(
            "Password was typed, but the clipboard could not be cleared"
        );
    }
#elif defined(__linux__)
    if (std::getenv("WAYLAND_DISPLAY") != nullptr) {
        throw std::runtime_error(
            "TACTO is not supported in Wayland sessions; use an X11 session"
        );
    }

    Display* display = XOpenDisplay(nullptr);
    if (display == nullptr) {
        throw std::runtime_error("Could not connect to the X11 display");
    }

    XTestApi test;
    try {
        LoadXTest(test);
    }
    catch (...) {
        XCloseDisplay(display);
        throw;
    }

    int eventBase = 0;
    int errorBase = 0;
    int majorVersion = 0;
    int minorVersion = 0;
    if (!test.queryExtension(
            display,
            &eventBase,
            &errorBase,
            &majorVersion,
            &minorVersion
        ))
    {
        XCloseDisplay(display);
        throw std::runtime_error("The X11 XTest extension is unavailable");
    }

    if (firstHalfLength >
        static_cast<std::size_t>(std::numeric_limits<int>::max()))
    {
        XCloseDisplay(display);
        throw std::length_error("Password is too large to auto-type");
    }

    const KeyCode control = XKeysymToKeycode(display, XK_Control_L);
    const KeyCode v = XKeysymToKeycode(display, XK_v);
    KeyCode shift = 0;
    std::vector<KeyStroke> keyStrokes;
    try {
        if (control == 0 || v == 0) {
            throw std::runtime_error(
                "The active keyboard layout cannot paste clipboard data"
            );
        }
        keyStrokes = PrepareX11KeyStrokes(
            display,
            password,
            firstHalfLength,
            length,
            shift
        );
    }
    catch (...) {
        XCloseDisplay(display);
        throw;
    }

    WaitForTargetWindow();
    const Window owner = XCreateSimpleWindow(
        display,
        DefaultRootWindow(display),
        0,
        0,
        1,
        1,
        0,
        0,
        0
    );
    if (owner == None) {
        XCloseDisplay(display);
        throw std::runtime_error("Could not create an X11 clipboard owner");
    }
    const Atom clipboard = XInternAtom(display, "CLIPBOARD", False);
    const Atom targets = XInternAtom(display, "TARGETS", False);
    const Atom utf8String = XInternAtom(display, "UTF8_STRING", False);

    XSetSelectionOwner(display, clipboard, owner, CurrentTime);
    XSync(display, False);
    if (XGetSelectionOwner(display, clipboard) != owner) {
        XDestroyWindow(display, owner);
        XCloseDisplay(display);
        throw std::runtime_error("Could not set the X11 clipboard");
    }

    try {
        PasteX11Clipboard(display, test, control, v);
        if (!WaitForX11ClipboardRequest(
                display,
                targets,
                utf8String,
                password,
                firstHalfLength
            ))
        {
            throw std::runtime_error(
                "The target application did not request clipboard data"
            );
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        TypeX11Password(display, test, keyStrokes, shift);
    }
    catch (...) {
        const bool clipboardCleared =
            ClearX11Clipboard(display, owner, clipboard);
        XDestroyWindow(display, owner);
        XCloseDisplay(display);
        if (!clipboardCleared) {
            throw std::runtime_error(
                "Auto-type failed and the clipboard could not be cleared"
            );
        }
        throw;
    }

    const bool clipboardCleared =
        ClearX11Clipboard(display, owner, clipboard);
    XDestroyWindow(display, owner);
    XCloseDisplay(display);
    if (!clipboardCleared) {
        throw std::runtime_error(
            "Password was typed, but the clipboard could not be cleared"
        );
    }
#else
    (void)firstHalfLength;
    throw std::runtime_error(
        "TACTO auto-type is not supported on this platform"
    );
#endif
}
