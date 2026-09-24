#pragma once

#include <string>

struct SecuritySettings {
    bool KDF_Argon2id = true;
    bool KDF_AES_KDF = false;

    int auto_lock_time_SECONDS = 5;

    bool Lock_On_System_Sleep = true;
    bool Lock_On_Window_minimize = true;
    bool Lock_On_Wifi_Connection = false;
    bool Lock_On_Suspicious_Bhevaior = false;
};

struct Ghost_Features {
    int Clear_Clipboard_timer_SECONDS = 40;
    bool clear_terminal_persitant_logs_on_exit = true;
    bool TCATO = true;
};

const SecuritySettings& GetSecuritySettings();
const Ghost_Features& GetGhostFeatures();
SecuritySettings& EditSecuritySettings();
Ghost_Features& EditGhostFeatures();
std::string GetSettingsSection(int option);
