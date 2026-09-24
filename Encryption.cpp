#include <sodium.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "Encryption.h"

namespace {

class SecureBuffer {
public:
    explicit SecureBuffer(std::size_t size)
        : size_(size),
          data_(static_cast<unsigned char*>(sodium_malloc(size)))
    {
        if (data_ == nullptr) {
            throw std::bad_alloc();
        }

        sodium_memzero(data_, size_);

        if (sodium_mlock(data_, size_) != 0) {
            sodium_free(data_);
            data_ = nullptr;
            size_ = 0;
            throw std::runtime_error("Failed to lock secure memory");
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
        if (this != &other) {
            Release();
            size_ = other.size_;
            data_ = other.data_;
            other.size_ = 0;
            other.data_ = nullptr;
        }

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

private:
    void Release() noexcept
    {
        if (data_ == nullptr) {
            return;
        }

        sodium_free(data_);
        data_ = nullptr;
        size_ = 0;
    }

    std::size_t size_;
    unsigned char* data_;
};


void ValidateSettings(const SecuritySettings& settings)
{
    if (!settings.KDF_Argon2id) {
        throw std::runtime_error("Argon2id is disabled in SecuritySettings");
    }

    if (settings.KDF_AES_KDF) {
        throw std::runtime_error("AES-KDF is enabled but is not implemented");
    }

}


SecureBuffer DeriveKey(
    const unsigned char* password,
    std::size_t passwordLength,
    const unsigned char* salt,
    const SecuritySettings& settings
)
{
    if (password == nullptr || passwordLength == 0) {
        throw std::invalid_argument("Password cannot be empty");
    }

    ValidateSettings(settings);

    SecureBuffer key(crypto_aead_xchacha20poly1305_ietf_KEYBYTES);

    if (crypto_pwhash(
            key.data(),
            key.size(),
            reinterpret_cast<const char*>(password),
            passwordLength,
            salt,
            crypto_pwhash_OPSLIMIT_MODERATE,
            crypto_pwhash_MEMLIMIT_MODERATE,
            crypto_pwhash_ALG_ARGON2ID13
        ) != 0)
    {
        throw std::runtime_error("Argon2id key derivation failed");
    }

    return key;
}

}


SecureSecret::SecureSecret(std::size_t capacity)
    : capacity_(capacity),
      data_(static_cast<unsigned char*>(sodium_malloc(capacity)))
{
    if (data_ == nullptr) {
        throw std::bad_alloc();
    }

    sodium_memzero(data_, capacity_);

    if (sodium_mlock(data_, capacity_) != 0) {
        sodium_free(data_);
        data_ = nullptr;
        capacity_ = 0;
        throw std::runtime_error("Failed to lock secure memory");
    }
}


SecureSecret::~SecureSecret()
{
    if (data_ != nullptr) {
        sodium_free(data_);
        data_ = nullptr;
    }
    size_ = 0;
    capacity_ = 0;
}


SecureSecret::SecureSecret(SecureSecret&& other) noexcept
    : size_(other.size_),
      capacity_(other.capacity_),
      data_(other.data_)
{
    other.size_ = 0;
    other.capacity_ = 0;
    other.data_ = nullptr;
}


SecureSecret& SecureSecret::operator=(SecureSecret&& other) noexcept
{
    if (this != &other) {
        if (data_ != nullptr) {
            sodium_free(data_);
        }

        size_ = other.size_;
        capacity_ = other.capacity_;
        data_ = other.data_;
        other.size_ = 0;
        other.capacity_ = 0;
        other.data_ = nullptr;
    }

    return *this;
}


unsigned char* SecureSecret::data()
{
    return data_;
}


const unsigned char* SecureSecret::data() const
{
    return data_;
}


std::size_t SecureSecret::size() const
{
    return size_;
}


std::size_t SecureSecret::capacity() const
{
    return capacity_;
}


void SecureSecret::Resize(std::size_t size)
{
    if (size > capacity_) {
        throw std::length_error("Secure secret capacity exceeded");
    }

    if (size < size_) {
        sodium_memzero(data_ + size, size_ - size);
    }

    size_ = size;
}


bool SecureSecret::Equals(const char* value) const
{
    if (value == nullptr) {
        return false;
    }

    const std::size_t valueSize = std::strlen(value);
    return valueSize == size_ &&
           sodium_memcmp(data_, value, size_) == 0;
}


void SecureSecret::Clear() noexcept
{
    if (data_ != nullptr) {
        sodium_memzero(data_, capacity_);
    }
    size_ = 0;
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
        throw std::runtime_error("libsodium initialization failed");
    }

    if (plaintext == nullptr && plaintextLength != 0) {
        throw std::invalid_argument("Plaintext pointer is null");
    }

    EncryptedData result;

    randombytes_buf(result.salt.data(), result.salt.size());
    randombytes_buf(result.nonce.data(), result.nonce.size());

    SecureBuffer key = DeriveKey(
        password,
        passwordLength,
        result.salt.data(),
        settings
    );

    result.ciphertext.reserve(
        plaintextLength + crypto_aead_xchacha20poly1305_ietf_ABYTES
    );
    result.ciphertext.resize(
        plaintextLength + crypto_aead_xchacha20poly1305_ietf_ABYTES
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
        sodium_memzero(result.ciphertext.data(), result.ciphertext.size());
        result.ciphertext.clear();
        throw std::runtime_error("XChaCha20-Poly1305 encryption failed");
    }

    result.ciphertext.resize(
        static_cast<std::size_t>(ciphertextLength)
    );

    return result;
}


SecureSecret Decrypt(
    const EncryptedData& encrypted,
    const unsigned char* password,
    std::size_t passwordLength,
    const SecuritySettings& settings
)
{
    if (sodium_init() < 0) {
        throw std::runtime_error("libsodium initialization failed");
    }

    if (encrypted.ciphertext.size() <
        crypto_aead_xchacha20poly1305_ietf_ABYTES)
    {
        throw std::invalid_argument("Ciphertext is too short");
    }

    SecureBuffer key = DeriveKey(
        password,
        passwordLength,
        encrypted.salt.data(),
        settings
    );

    const std::size_t plaintextSize =
        encrypted.ciphertext.size() -
        crypto_aead_xchacha20poly1305_ietf_ABYTES;
    SecureSecret plaintext(std::max<std::size_t>(1, plaintextSize));
    plaintext.Resize(plaintextSize);

    unsigned long long plaintextLength = 0;

    if (crypto_aead_xchacha20poly1305_ietf_decrypt(
            plaintext.data(),
            &plaintextLength,
            nullptr,
            encrypted.ciphertext.data(),
            encrypted.ciphertext.size(),
            nullptr,
            0,
            encrypted.nonce.data(),
            key.data()
        ) != 0)
    {
        throw std::runtime_error("Password verification failed");
    }

    plaintext.Resize(static_cast<std::size_t>(plaintextLength));
    return plaintext;
}


SecureSecret ReadPassword(const std::string& prompt)
{
    SecureBuffer password(256);
    std::size_t length = 0;
    char character = '\0';

    std::cout << prompt;

    while (std::cin.get(character)) {
        if (character == '\n') {
            break;
        }

        if (length + 1 >= password.size()) {
            while (std::cin.get(character) && character != '\n') {
            }
            throw std::invalid_argument("Password is too long");
        }

        password.data()[length] =
            static_cast<unsigned char>(character);
        ++length;
    }

    if (std::cin.bad() || (std::cin.eof() && length == 0)) {
        throw std::runtime_error("Password input failed");
    }

    if (length == 0) {
        throw std::invalid_argument("Password cannot be empty");
    }

    SecureSecret result(256);
    result.Resize(length);
    std::memcpy(result.data(), password.data(), length);
    return result;
}


SecureSecret GeneratePassword(std::size_t length)
{
    if (sodium_init() < 0) {
        throw std::runtime_error("libsodium initialization failed");
    }

    if (length < 16 || length > 256) {
        throw std::invalid_argument("Generated password length is invalid");
    }

    static constexpr char Characters[] =
        "ABCDEFGHJKLMNPQRSTUVWXYZ"
        "abcdefghijkmnopqrstuvwxyz"
        "23456789"
        "!@#$%^&*()-_=+";

    constexpr std::size_t CharacterCount = sizeof(Characters) - 1;
    SecureSecret password(length);
    password.Resize(length);

    for (std::size_t index = 0; index < length; ++index) {
        password.data()[index] =
            static_cast<unsigned char>(
                Characters[randombytes_uniform(CharacterCount)]
            );
    }

    return password;
}


void WriteSecret(
    std::ostream& output,
    const SecureSecret& secret
)
{
    output.write(
        reinterpret_cast<const char*>(secret.data()),
        static_cast<std::streamsize>(secret.size())
    );
}
