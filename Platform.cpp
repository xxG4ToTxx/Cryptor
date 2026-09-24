#include "Platform.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace Platform {

void ClearScreen() {
#ifdef _WIN32
    std::system("cls");
#endif

    std::cout
        << "\x1b[2J"
        << "\x1b[3J"
        << "\x1b[H";
    std::cout.flush();
}

bool ClearClipboard()
{
#ifdef _WIN32
    return std::system(
        "powershell -NoProfile -Command \"Set-Clipboard -Value $null\" >nul 2>&1"
    ) == 0;
#elif __APPLE__
    return std::system("printf '' | pbcopy 2>/dev/null") == 0;
#else
    if (std::system("command -v wl-copy >/dev/null 2>&1") == 0) {
        return std::system("printf '' | wl-copy 2>/dev/null") == 0;
    }

    if (std::system("command -v xclip >/dev/null 2>&1") == 0) {
        return std::system(
            "printf '' | xclip -selection clipboard"
            " 2>/dev/null"
        ) == 0;
    }

    if (std::system("command -v xsel >/dev/null 2>&1") == 0) {
        return std::system(
            "printf '' | xsel --clipboard --input"
            " 2>/dev/null"
        ) == 0;
    }

    return false;
#endif
}

std::filesystem::path VaultDirectory() {
#ifdef _WIN32
    const char* home = std::getenv("USERPROFILE");
    if (home == nullptr) {
        const char* drive = std::getenv("HOMEDRIVE");
        const char* path = std::getenv("HOMEPATH");
        if (drive != nullptr && path != nullptr) {
            return std::filesystem::path(std::string(drive) + path) / "Documents" / "cryptor";
        }
    }
#else
    const char* home = std::getenv("HOME");
#endif

    if (home != nullptr) {
        return std::filesystem::path(home) / "Documents" / "cryptor";
    }

    return std::filesystem::current_path() / "cryptor";
}

}