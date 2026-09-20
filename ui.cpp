#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <limits>



#include "ui.h"
#include "VaultManager.h"
#include "Encryption.h"
#include "settings.h"




void ShowFooter(){
std::cout << "-_-_-_-CRYPTOR-_-_-_-\n";

};


std::string CreatePasswordMenu(){
  
system("cls");
ShowFooter();
if(!IsVaultOpen){
    std::cout << "No vault currently opened. create or open one\n enter 1 to create one\n  > ";
    int option;
    std::cin >> option;
    if(option == 1){


        

    }
    else if(option == 2){}
    else{};
    
};

system("cls");
std::cout << "Enter Password to store\n  > ";
InvokePasswordPrompt();






return "SUCCESS";
}


std::string MainMenu(){

system("cls");
std::cout << "-_-_-_-CRYPTOR-_-_-_-\n 1: New Password\n 2: Open vault\n 3: Settings\n 4: Verify Integrity\n 5: Exit\n   > ";



    int Option;
    int count = 3;
     while (!(std::cin >> Option)) {

            //check if user has failed more than 3 times then reset the terminal to make it more organied
        if(count < 1){
            system("cls");
            std::cout << "-_-_-_-CRYPTOR-_-_-_-\n 1: New Password\n 2: Open vault\n 3: Settings\n 4: Verify Integrity\n 5: Exit\n   > ";
                count = 3;
        };


        // 1. Reset the error state flgs on cin
        std::cin.clear();

        // 2. Discard invalid chracters still sittng in the input buffer
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        std::cout << "(error: invalid selection)\n >";
        count--;
    };
    switch(Option){
        case 1:
        CreatePasswordMenu();
        break;
        case 2:
        break;
        case 3:
        OpenSettings();
        break;
        case 4:

        break;
        case 5:

        break;
    };


return "finished";
};