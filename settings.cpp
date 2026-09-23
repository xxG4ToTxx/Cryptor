#include <iostream>


struct SecuritySettings{
    bool KDF_Argon2id = true;
    bool KDF_AES_KDF = false;
    bool KDF_Secretbox = true;


    int auto_lock_time_SECONDS = 5;


    bool Lock_On_System_Sleep = true;
    bool Lock_On_Window_minimize = true;
    bool Lock_On_Wifi_Connection = false;
    bool Lock_On_Suspicious_Bhevaior = false;

};

struct Ghost_Features{

    int Clear_Clipboard_timer_SECONDS = 40;
    bool clear_terminal_persitant_logs_on_exit = true;
    bool TCATO = true;

};



int OpenSettings(){

    int option;
    std::cout << "1: Security settings\n2: Ghost features\n3: cloud/backups\n4: other settings\n  > ";
    std::cin >> option;
    switch(option){
        case 1:
            system("cls");
            

            
            break;
        case 2:
            std::cout << "\n";
            break;
        case 3:
            std::cout << "\n";
            break;
        case 4:
            std::cout << "\n";
    }
    





return 0;
};