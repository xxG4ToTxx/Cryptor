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
    char password[256]

    sodium_memzero(password, sizeof(password));


}
