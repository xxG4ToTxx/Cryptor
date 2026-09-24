#include <iostream>
#include <limits>
#include <string>

#include "Encryption.h"
#include "Platform.h"
#include "VaultManager.h"
#include "cleanup.h"
#include "settings.h"
#include "ui.h"

bool IsVaultOpen = false;

namespace {

std::string CurrentVault;
bool CurrentDecoy = false;
bool UnlockWritable = false;


void WaitForInput()
{
    std::cout << "\nPress Enter to continue...";
    std::cin.get();
}


bool ReadName(const std::string& prompt, std::string& name)
{
    std::cout << prompt;
    std::getline(std::cin, name);
    return !std::cin.fail() && !name.empty();
}


bool CreateVaultMenu()
{
    std::string name;
    if (!ReadName("Vault name: ", name)) {
        return false;
    }

    const std::string result = CreateNewVault(name);
    if (result != "SUCCESS") {
        std::cout << "Unable to create vault: " << result << '\n';
        return false;
    }

    CurrentVault = name;
    IsVaultOpen = true;
    CurrentDecoy = false;
    UnlockWritable = true;
    std::cout << "Vault created and opened.\n";
    return true;
}


bool OpenVaultMenu()
{
    std::string name;
    if (!ReadName("Vault name: ", name)) {
        return false;
    }

    if (!VerifyVault(name)) {
        std::cout << "Vault is missing or invalid.\n";
        return false;
    }

    CurrentDecoy = false;
    int unlockResult = 0;
    SecureSecret storedPassword;
    SecureSecret masterPassword;
    if (HasStoredPassword(name) || HasDecoyPassword(name)) {
        try {
            masterPassword = ReadPassword("Master password: ");
            unlockResult = LoadPassword(
                name,
                masterPassword,
                storedPassword,
                CurrentDecoy
            );
        }
        catch (const std::exception& error) {
            std::cout << error.what() << '\n';
            return false;
        }
    }

    CurrentVault = name;
    IsVaultOpen = true;
    UnlockWritable = unlockResult != 2;

    Platform::ClearScreen();
    ShowFooter();
    std::cout << "Vault opened.\n\n"
              << "Stored entries:\n";
    if (storedPassword.size() == 0) {
        std::cout << "(none)\n";
    }
    else {
        WriteSecret(std::cout, storedPassword);
        std::cout << '\n';
    }

    WaitForInput();
    Platform::ClearScreen();
    return true;
}

}


void LockSession()
{
    IsVaultOpen = false;
    CurrentVault.clear();
    CurrentDecoy = false;
    UnlockWritable = false;
}


void ShowFooter()
{
    std::cout << "-_-_-_-CRYPTOR-_-_-_-\n";
}


std::string CreatePasswordMenu()
{
    Platform::ClearScreen();
    ShowFooter();

    if (!IsVaultOpen) {
        std::cout << "1: Create vault\n2: Open vault\n3: Cancel\n  > ";

        int option = 0;
        if (!(std::cin >> option)) {
            std::cin.clear();
            std::cin.ignore(
                std::numeric_limits<std::streamsize>::max(),
                '\n'
            );
            return "INVALID_SELECTION";
        }
        std::cin.ignore(
            std::numeric_limits<std::streamsize>::max(),
            '\n'
        );

        if (option == 1) {
            CreateVaultMenu();
        }
        else if (option == 2) {
            OpenVaultMenu();
        }
        else {
            return "CANCELLED";
        }
    }

    if (!IsVaultOpen) {
        return "NO_VAULT";
    }

    try {
        if (!UnlockWritable) {
            std::cout << "This vault view is read-only.\n";
            return "READ_ONLY";
        }

        SecureSecret masterPassword =
            ReadPassword("Master password: ");
        SecureSecret password = ReadPassword(
            "Password to store or /generate_password: "
        );

        if (password.Equals("/generate_password")) {
            password = GeneratePassword();
            std::cout << "Generated password: ";
            WriteSecret(std::cout, password);
            std::cout << '\n';
        }

        const int result = SavePassword(
            CurrentVault,
            password,
            masterPassword,
            CurrentDecoy
        );
        if (result != 0) {
            std::cout << "Password could not be saved.\n";
            return "SAVE_FAILED";
        }

        std::cout << "Password saved.\n";
        return "SUCCESS";
    }
    catch (const std::exception& error) {
        std::cout << error.what() << '\n';
        return "INPUT_FAILED";
    }
}


std::string MainMenu()
{
    bool running = true;

    while (running && !Cleanup::TerminationRequested()) {
        Platform::ClearScreen();
        ShowFooter();
        std::cout << "1: New Password\n"
                  << "2: Open vault\n"
                  << "3: Settings\n"
                  << "4: Verify Integrity\n"
                  << "5: Exit\n"
                  << "Vault: "
                  << (IsVaultOpen ? CurrentVault : "none")
                  << "\n  > ";

        int option = 0;
        if (!(std::cin >> option)) {
            std::cin.clear();
            std::cin.ignore(
                std::numeric_limits<std::streamsize>::max(),
                '\n'
            );
            std::cout << "Invalid selection.\n";
            WaitForInput();
            continue;
        }
        std::cin.ignore(
            std::numeric_limits<std::streamsize>::max(),
            '\n'
        );

        switch (option) {
        case 1:
            CreatePasswordMenu();
            WaitForInput();
            break;
        case 2:
            Platform::ClearScreen();
            ShowFooter();
            if (LookForVaults() == "NO_VAULTS") {
                std::cout << "No vaults found.\n";
            }
            if (!OpenVaultMenu()) {
                WaitForInput();
            }
            break;
        case 3:
            Platform::ClearScreen();
            ShowFooter();
            OpenSettings();
            WaitForInput();
            break;
        case 4:
            Platform::ClearScreen();
            ShowFooter();
            if (!IsVaultOpen || !VerifyVault(CurrentVault)) {
                std::cout << "No valid vault is open.\n";
            }
            else {
                std::cout << "Vault integrity check passed.\n";
            }
            WaitForInput();
            break;
        case 5:
            running = false;
            break;
        default:
            std::cout << "Invalid selection.\n";
            WaitForInput();
            break;
        }
    }

    LockSession();
    return "EXITED";
}





int OpenSettings()
{
    std::cout << "1: Security settings\n"
              << "2: Ghost features\n"
              << "3: Cloud and backups\n"
              << "4: Other settings\n"
              << "5: Configure decoy password\n"
              << "6: Remove decoy password\n"
              << "  > ";

    int option = 0;
    if (!(std::cin >> option)) {
        std::cin.clear();
        std::cin.ignore(
            std::numeric_limits<std::streamsize>::max(),
            '\n'
        );
        std::cout << "Invalid selection.\n";
        return 1;
    }

    std::cin.ignore(
        std::numeric_limits<std::streamsize>::max(),
        '\n'
    );

    if (option == 1) {
        SecuritySettings& settings = EditSecuritySettings();
        std::cout << "1: Argon2id ("
                  << (settings.KDF_Argon2id ? "on" : "off") << ")\n"
                  << "2: Auto-lock seconds ("
                  << settings.auto_lock_time_SECONDS << ")\n"
                  << "3: Lock on system sleep ("
                  << (settings.Lock_On_System_Sleep ? "on" : "off") << ")\n"
                  << "4: Lock on window minimize ("
                  << (settings.Lock_On_Window_minimize ? "on" : "off")
                  << ")\n"
                  << "5: Lock on Wi-Fi connection ("
                  << (settings.Lock_On_Wifi_Connection ? "on" : "off")
                  << ")\n"
                  << "6: Lock on suspicious behavior ("
                  << (settings.Lock_On_Suspicious_Bhevaior ? "on" : "off")
                  << ")\n"
                  << "7: Back\n  > ";

        int setting = 0;
        if (!(std::cin >> setting)) {
            std::cin.clear();
            std::cin.ignore(
                std::numeric_limits<std::streamsize>::max(),
                '\n'
            );
            return 1;
        }
        std::cin.ignore(
            std::numeric_limits<std::streamsize>::max(),
            '\n'
        );

        if (setting == 1) {
            settings.KDF_Argon2id = !settings.KDF_Argon2id;
        }
        else if (setting == 2) {
            std::cout << "Auto-lock seconds: ";
            if (!(std::cin >> settings.auto_lock_time_SECONDS) ||
                settings.auto_lock_time_SECONDS < 0)
            {
                std::cin.clear();
                std::cin.ignore(
                    std::numeric_limits<std::streamsize>::max(),
                    '\n'
                );
                return 1;
            }
            std::cin.ignore(
                std::numeric_limits<std::streamsize>::max(),
                '\n'
            );
        }
        else if (setting >= 3 && setting <= 6) {
            bool* selected = nullptr;
            if (setting == 3) {
                selected = &settings.Lock_On_System_Sleep;
            }
            else if (setting == 4) {
                selected = &settings.Lock_On_Window_minimize;
            }
            else if (setting == 5) {
                selected = &settings.Lock_On_Wifi_Connection;
            }
            else {
                selected = &settings.Lock_On_Suspicious_Bhevaior;
            }
            *selected = !*selected;
        }

        return 0;
    }

    if (option == 2) {
        Ghost_Features& features = EditGhostFeatures();
        std::cout << "1: Clipboard clear seconds ("
                  << features.Clear_Clipboard_timer_SECONDS << ")\n"
                  << "2: Clear terminal scrollback on exit ("
                  << (features.clear_terminal_persitant_logs_on_exit
                          ? "on"
                          : "off")
                  << ")\n"
                  << "3: Clear temporary artifacts on exit ("
                  << (features.TCATO ? "on" : "off") << ")\n"
                  << "4: Back\n  > ";

        int setting = 0;
        if (!(std::cin >> setting)) {
            std::cin.clear();
            std::cin.ignore(
                std::numeric_limits<std::streamsize>::max(),
                '\n'
            );
            return 1;
        }
        std::cin.ignore(
            std::numeric_limits<std::streamsize>::max(),
            '\n'
        );

        if (setting == 1) {
            std::cout << "Clipboard clear seconds: ";
            if (!(std::cin >> features.Clear_Clipboard_timer_SECONDS) ||
                features.Clear_Clipboard_timer_SECONDS < 0)
            {
                std::cin.clear();
                std::cin.ignore(
                    std::numeric_limits<std::streamsize>::max(),
                    '\n'
                );
                return 1;
            }
            std::cin.ignore(
                std::numeric_limits<std::streamsize>::max(),
                '\n'
            );
        }
        else if (setting == 2) {
            features.clear_terminal_persitant_logs_on_exit =
                !features.clear_terminal_persitant_logs_on_exit;
        }
        else if (setting == 3) {
            features.TCATO = !features.TCATO;
        }

        return 0;
    }

    if (option == 5) {
        if (!IsVaultOpen) {
            std::cout << "Open a vault first.\n";
            return 1;
        }

        try {
            SecureSecret decoyPassword =
                ReadPassword("Decoy password: ");
            const int result = ConfigureDecoyPassword(
                CurrentVault,
                decoyPassword
            );
            std::cout << (result == 0
                              ? "Decoy password configured.\n"
                              : "Decoy password could not be configured.\n");
            return result == 0 ? 0 : 1;
        }
        catch (const std::exception& error) {
            std::cout << error.what() << '\n';
            return 1;
        }
    }

    if (option == 6) {
        if (!IsVaultOpen) {
            std::cout << "Open a vault first.\n";
            return 1;
        }

        const int result = RemoveDecoyPassword(CurrentVault);
        std::cout << (result == 0
                          ? "Decoy password removed.\n"
                          : "Decoy password could not be removed.\n");
        return result == 0 ? 0 : 1;
    }

    if (option < 1 || option > 4) {
        std::cout << "Invalid selection.\n";
        return 1;
    }

    std::cout << GetSettingsSection(option);
    return 0;
}