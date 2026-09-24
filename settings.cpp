#include <sstream>

#include "settings.h"

namespace {

SecuritySettings securitySettings;
Ghost_Features ghostFeatures;

}


const SecuritySettings& GetSecuritySettings()
{
    return securitySettings;
}


const Ghost_Features& GetGhostFeatures()
{
    return ghostFeatures;
}


SecuritySettings& EditSecuritySettings()
{
    return securitySettings;
}


Ghost_Features& EditGhostFeatures()
{
    return ghostFeatures;
}


std::string GetSettingsSection(int option)
{
    std::ostringstream output;

    switch (option) {
    case 1:
        output << "Argon2id: "
               << (securitySettings.KDF_Argon2id ? "enabled" : "disabled")
               << "\n"
               << "XChaCha20-Poly1305: enabled\n"
               << "Auto-lock: "
               << securitySettings.auto_lock_time_SECONDS
               << " seconds\n";
        break;
    case 2:
        output << "Clear clipboard timer: "
               << ghostFeatures.Clear_Clipboard_timer_SECONDS
               << " seconds\n"
               << "Clear terminal history on exit: "
               << (ghostFeatures.clear_terminal_persitant_logs_on_exit
                       ? "enabled"
                       : "disabled")
               << "\n";
        break;
    case 3:
        output << "Cloud and backups are not configured.\n";
        break;
    case 4:
        output << "No other settings are configured.\n";
        break;
    default:
        output << "Invalid selection.\n";
        break;
    }

    return output.str();
}
