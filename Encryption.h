#pragma once

#include <sodium.h>

#include <array>
#include <cstddef>
#include <string>
#include <vector>

#include "settings.h"


struct EncryptedData {
    unsigned char version = 1;

    std::array<
        unsigned char,
        crypto_pwhash_SALTBYTES
    > salt{};

    std::array<
        unsigned char,
        crypto_aead_xchacha20poly1305_ietf_NPUBBYTES
    > nonce{};

    std::vector<unsigned char> ciphertext;
};


EncryptedData Encrypt(
    const unsigned char* plaintext,
    std::size_t plaintextLength,
    const unsigned char* password,
    std::size_t passwordLength,
    const SecuritySettings& settings
);


void InvokePasswordPrompt();
