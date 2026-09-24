#include "cleanup.h"

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <string>

#include "Platform.h"
#include "VaultManager.h"
#include "settings.h"
#include "ui.h"

namespace {

volatile std::sig_atomic_t terminationRequested = 0;
std::atomic_flag cleanupComplete = ATOMIC_FLAG_INIT;


void HandleTermination(int)
{
    terminationRequested = 1;
}

}

namespace Cleanup {

void Install()
{
    std::signal(SIGINT, HandleTermination);
    std::signal(SIGTERM, HandleTermination);
    std::atexit(Run);
}


bool TerminationRequested()
{
    return terminationRequested != 0;
}


void Run()
{
    if (cleanupComplete.test_and_set()) {
        return;
    }

    LockSession();

    if (GetGhostFeatures().TCATO) {
        try {
            const std::filesystem::path root = Platform::VaultDirectory();
            if (std::filesystem::is_directory(root)) {
                for (const auto& entry :
                     std::filesystem::recursive_directory_iterator(root))
                {
                    if (entry.is_regular_file() &&
                        entry.path().extension() == ".tmp")
                    {
                        std::filesystem::remove(entry.path());
                    }
                }
            }
        }
        catch (...) {
        }
    }

    if (GetGhostFeatures().Clear_Clipboard_timer_SECONDS >= 0) {
        Platform::ClearClipboard();
    }

    if (GetGhostFeatures().clear_terminal_persitant_logs_on_exit) {
        Platform::ClearScreen();
    }
}

}