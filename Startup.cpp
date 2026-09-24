#include <iostream>
#include <string>
#include <sodium.h>

#include "Platform.h"
#include "VaultManager.h"

std::string StartUp()
{
    if (InitializeVaultDirectory() != "PASSED CHECKS") {
        Platform::ClearScreen();
        return "Failed to initialize";
    }

    if (sodium_init() < 0) {
        std::cerr << "Libsodium failed to initialize.\n";
        return "Failed to initialize";
    }

    return "SUCCESS";
}
