#pragma once

#include <sodium.h>

#include <array>
#include <cstddef>
#include <iosfwd>
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


class SecureSecret {
public:
    explicit SecureSecret(std::size_t capacity = 256);
    ~SecureSecret();

    SecureSecret(const SecureSecret&) = delete;
    SecureSecret& operator=(const SecureSecret&) = delete;
    SecureSecret(SecureSecret&& other) noexcept;
    SecureSecret& operator=(SecureSecret&& other) noexcept;

    unsigned char* data();
    const unsigned char* data() const;
    std::size_t size() const;
    std::size_t capacity() const;
    void Resize(std::size_t size);
    bool Equals(const char* value) const;
    void Clear() noexcept;

private:
    std::size_t size_ = 0;
    std::size_t capacity_ = 0;
    unsigned char* data_ = nullptr;
};


EncryptedData Encrypt(
    const unsigned char* plaintext,
    std::size_t plaintextLength,
    const unsigned char* password,
    std::size_t passwordLength,
    const SecuritySettings& settings
);


SecureSecret Decrypt(
    const EncryptedData& encrypted,
    const unsigned char* password,
    std::size_t passwordLength,
    const SecuritySettings& settings
);


SecureSecret ReadPassword(
    const std::string& prompt
);


SecureSecret GeneratePassword(std::size_t length = 24);


void WriteSecret(
    std::ostream& output,
    const SecureSecret& secret
);
