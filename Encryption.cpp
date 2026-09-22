#include <sodium.h>

#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "settings.h"
#include "ui.h"
#include "Encryption.h"

namespace {

using Key = std::array<
    unsigned char,
    crypto_aead_xchacha20poly1305_ietf_KEYBYTES
>;

using Nonce = std::array<
    unsigned char,
    crypto_aead_xchacha20poly1305_ietf_NPUBBYTES
>;

using Salt = std::array<
    unsigned char,
    crypto_pwhash_SALTBYTES
>;


class SecureBuffer {
public:
    explicit SecureBuffer(std::size_t size)
        : size_(size),
          data_(static_cast<unsigned char*>(sodium_malloc(size)))
    {
        if (data_ == nullptr) {
            throw std::bad_alloc();
        }

        sodium_memzero(
            data_,
            size_
        );

        if (sodium_mlock(data_, size_) != 0) {
            sodium_memzero(
                data_,
                size_
            );

            sodium_free(data_);

            data_ = nullptr;
            size_ = 0;

            throw std::runtime_error(
                "Failed to lock secure memory"
            );
        }
    }

    ~SecureBuffer()
    {
        Release();
    }

    SecureBuffer(const SecureBuffer&) = delete;
    SecureBuffer& operator=(const SecureBuffer&) = delete;

    SecureBuffer(SecureBuffer&& other) noexcept
        : size_(other.size_),
          data_(other.data_)
    {
        other.size_ = 0;
        other.data_ = nullptr;
    }

    SecureBuffer& operator=(SecureBuffer&& other) noexcept
    {
        if (this == &other) {
            return *this;
        }

        Release();

        size_ = other.size_;
        data_ = other.data_;

        other.size_ = 0;
        other.data_ = nullptr;

        return *this;
    }

    unsigned char* data()
    {
        return data_;
    }

    const unsigned char* data() const
    {
        return data_;
    }

    std::size_t size() const
    {
        return size_;
    }

    void Clear() noexcept
    {
        if (data_ != nullptr) {
            sodium_memzero(
                data_,
                size_
            );
        }
    }

private:
    void Release() noexcept
    {
        if (data_ == nullptr) {
            return;
        }

        sodium_memzero(
            data_,
            size_
        );

        sodium_munlock(
            data_,
            size_
        );

        sodium_free(
            data_
        );

        data_ = nullptr;
        size_ = 0;
    }

private:
    std::size_t size_;
    unsigned char* data_;
};


class TerminalCleanup {
public:
    explicit TerminalCleanup(bool enabled)
        : enabled_(enabled)
    {
    }

    ~TerminalCleanup()
    {
        if (enabled_) {
            Clear();
        }
    }

    TerminalCleanup(const TerminalCleanup&) = delete;
    TerminalCleanup& operator=(const TerminalCleanup&) = delete;

    static void Clear() noexcept
    {
        std::cout
            << "\x1b[2J"
            << "\x1b[3J"
            << "\x1b[H";

        std::cout.flush();
    }

private:
    bool enabled_;
};


std::string HexEncode(
    const unsigned char* data,
    std::size_t size
)
{
    if (data == nullptr || size == 0) {
        return {};
    }

    std::string result(
        size * 2 + 1,
        '\0'
    );

    sodium_bin2hex(
        result.data(),
        result.size(),
        data,
        size
    );

    result.resize(
        size * 2
    );

    return result;
}

}


EncryptedData Encrypt(
    const unsigned char* plaintext,
    std::size_t plaintextLength,
    const unsigned char* password,
    std::size_t passwordLength,
    const SecuritySettings& settings
)
{
    if (sodium_init() < 0) {
        throw std::runtime_error(
            "libsodium initialization failed"
        );
    }

    if (plaintext == nullptr && plaintextLength != 0) {
        throw std::invalid_argument(
            "Plaintext pointer is null"
        );
    }

    if (password == nullptr || passwordLength == 0) {
        throw std::invalid_argument(
            "Password cannot be empty"
        );
    }

    if (!settings.KDF_Argon2id) {
        throw std::runtime_error(
            "Argon2id is disabled in SecuritySettings"
        );
    }

    if (settings.KDF_AES_KDF) {
        throw std::runtime_error(
            "AES-KDF is enabled but is not implemented"
        );
    }

    if (!settings.KDF_Secretbox) {
        throw std::runtime_error(
            "Secretbox encryption is disabled"
        );
    }

    EncryptedData result;

    randombytes_buf(
        result.salt.data(),
        result.salt.size()
    );

    randombytes_buf(
        result.nonce.data(),
        result.nonce.size()
    );

    SecureBuffer key(
        crypto_aead_xchacha20poly1305_ietf_KEYBYTES
    );

    if (crypto_pwhash(
            key.data(),
            key.size(),
            reinterpret_cast<const char*>(password),
            passwordLength,
            result.salt.data(),
            crypto_pwhash_OPSLIMIT_MODERATE,
            crypto_pwhash_MEMLIMIT_MODERATE,
            crypto_pwhash_ALG_ARGON2ID13
        ) != 0)
    {
        throw std::runtime_error(
            "Argon2id key derivation failed"
        );
    }

    result.ciphertext.resize(
        plaintextLength +
        crypto_aead_xchacha20poly1305_ietf_ABYTES
    );

    unsigned long long ciphertextLength = 0;

    if (crypto_aead_xchacha20poly1305_ietf_encrypt(
            result.ciphertext.data(),
            &ciphertextLength,
            plaintext,
            plaintextLength,
            nullptr,
            0,
            nullptr,
            result.nonce.data(),
            key.data()
        ) != 0)
    {
        if (!result.ciphertext.empty()) {
            sodium_memzero(
                result.ciphertext.data(),
                result.ciphertext.size()
            );

            result.ciphertext.clear();
        }

        throw std::runtime_error(
            "XChaCha20-Poly1305 encryption failed"
        );
    }

    result.ciphertext.resize(
        static_cast<std::size_t>(
            ciphertextLength
        )
    );

    key.Clear();

    return result;
}


void InvokePasswordPrompt()
{
    if (sodium_init() < 0) {
        std::cerr
            << "Could not initialize libsodium!\n";

        return;
    }

    SecuritySettings securitySettings;
    Ghost_Features ghostFeatures;

    TerminalCleanup terminalCleanup(
        ghostFeatures.clear_terminal_persitant_logs_on_exit
    );

    SecureBuffer password(256);

    std::cout
        << "Enter password: ";

    std::cin >> std::ws;

    std::cin.getline(
        reinterpret_cast<char*>(password.data()),
        static_cast<std::streamsize>(
            password.size()
        )
    );

    if (std::cin.fail()) {
        std::cin.clear();

        std::cerr
            << "Password is too long.\n";

        return;
    }

    const std::size_t passwordLength =
        strnlen(
            reinterpret_cast<const char*>(
                password.data()
            ),
            password.size()
        );

    if (passwordLength == 0) {
        std::cerr
            << "Password cannot be empty.\n";

        return;
    }

    try {

        EncryptedData encrypted = Encrypt(
            password.data(),
            passwordLength,
            password.data(),
            passwordLength,
            securitySettings
        );

        if (ghostFeatures.TCATO) {
            return;
        }

    }
    catch (const std::exception& e) {

        std::cerr
            << "Encryption error: "
            << e.what()
            << '\n';
    }
}
