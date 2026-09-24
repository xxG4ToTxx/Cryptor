#include <chrono>
#include <thread>

#include "Platform.h"
#include "cleanup.h"
#include "startup.h"
#include "ui.h"

int main()
{
    Cleanup::Install();

    for (int attempt = 0; attempt < 3; ++attempt) {
        Platform::ClearScreen();
        if (StartUp() == "SUCCESS") {
            Platform::ClearScreen();
            return MainMenu() == "EXITED" ? 0 : 1;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    return 1;
}
