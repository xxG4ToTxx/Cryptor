#include <string>
#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>
#include <vector>
#include <iomanip>
#include <sodium.h>

#include "ui.h"

void InvokePasswordPrompt() {
    if (sodium_init() < 0) { 
        std::cerr << "Could not initialize libsodium!\n"; 
        return;
    }

    char password[256];
    std::cout << "Enter password: ";
    std::cin.getline(password, sizeof(password));

    unsigned char salt[crypto_pwhash_SALTBYTES];
    randombytes_buf(salt, sizeof(salt));

    unsigned char key[crypto_secretbox_KEYBYTES];

    if (crypto_pwhash(key, sizeof(key), password, strlen(password), salt,
                      crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE,
                      crypto_pwhash_ALG_DEFAULT) != 0) {
        std::cerr << "Key derivation failed!\n";
        return;
    }

    unsigned char nonce[crypto_secretbox_NONCEBYTES];
    randombytes_buf(nonce, sizeof(nonce));

    size_t encrypted_len = strlen(password) + crypto_secretbox_MACBYTES;
    std::vector<unsigned char> encrypted_password(encrypted_len);

    crypto_secretbox_easy(encrypted_password.data(), (const unsigned char*)password, strlen(password), nonce, key);

    std::cout << "\nEncrypted Password (Hex): ";
    for (size_t i = 0; i < encrypted_len; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)encrypted_password[i];
    }

    std::cout << "\nSalt (Hex): ";
    for (size_t i = 0; i < sizeof(salt); i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)salt[i];
    }

    std::cout << "\nDerived Key (Hex): ";
    for (size_t i = 0; i < sizeof(key); i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)key[i];
    }
    std::cout << std::dec << "\n";

    sodium_memzero(password, sizeof(password));
    sodium_memzero(salt, sizeof(salt));
    sodium_memzero(key, sizeof(key));
    sodium_memzero(nonce, sizeof(nonce));
}
