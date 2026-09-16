#include <string>
#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>
#include <vector>
#include <sodium.h>

#include "ui.h"

\




void InvokePasswordPrompt(){

    unsigned char password[256];

   std::cin >> password;

   if(sodium_mlock(password, sizeof(password)) != 0){
    system("cls")
    std::cerr << "Warning: could not lock memory (check ulimits)";
   }


}








/* 
 if (sodium_init() < 0) {
    std::cerr << "libsodium init failed\n";
    return "ERROR";
};
*/



/*

char *Input = (char *) sodium_malloc(128);
std::cin.ignore();
std::cin.getline(Input, 127);

if(std::cin.fail()){
std::cin.clear();
std::cerr << "password failed to be stored, make sure password is below 127 characters!";
sodium_free(Input);
std::this_thread::sleep_for(std::chrono::seconds(3));
system("cls");
}
*/