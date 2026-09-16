#include <string>
#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>
#include <vector>
#include <sodium.h>

#include "ui.h"

void InvokePasswordPrompt(){
    if (sodium_init() < 0) { 
        std::cerr << "Could not initialize libsodium!\n"; 
        return; 
    }

    // Allocate secure memory (sodium_malloc automatically handles mlock/locking)
    char* password = (char*) sodium_malloc(256);
    if (password == nullptr) {
        std::cerr << "Memory allocation failed!\n";
        return;
    }

    // CRITICAL: Clear out any leftover newlines from previous std::cin inputs
    if (std::cin.rdbuf()->in_avail() > 0 || std::cin.fail()) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
    }

    bool valid_input = false;

    while(!valid_input){
        std::cout << "Enter password: ";
        std::cin.getline(password, 256);

        if (std::cin.fail()) {
            std::cin.clear(); // Clear error flags
            std::cin.ignore(10000, '\n'); // Purge remaining extra characters
            system("cls");
            std::cout << "Your password is too big!\n";
            std::this_thread::sleep_for(std::chrono::seconds(3));
            system("cls");
        } else {
            valid_input = true;
        }
    }

    // --- Place your cryptographic operations here ---
    std::cout << "Password securely accepted!\n"; 

    // Securely zeros out and unlocks memory automatically
    sodium_free(password); 
}
