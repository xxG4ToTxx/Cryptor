#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "Encryption.h"
#include "Platform.h"
#include "VaultManager.h"

namespace fs = std::filesystem;

bool DirectoryExists = false;
bool isFolder = false;

struct Vault {
    std::string name;
    int ID = 0;
    std::string creation_date;
    fs::path Path;
};

namespace {

constexpr const char* VaultMetadataFile = "vault.meta";
constexpr const char* VaultDataFile = "vault.cryptor";
constexpr const char* PasswordFile = "password.cryptor";
constexpr const char* DecoyPasswordFile = "decoy.cryptor";
constexpr const char* PasswordTempFile = "password.cryptor.tmp";
constexpr std::uint8_t CurrentVersion = 1;
constexpr std::uint64_t MaximumCiphertextSize = 64 * 1024 * 1024;


fs::path VaultRoot()
{
    return Platform::VaultDirectory();
}


bool IsSafeVaultName(const std::string& name)
{
    if (name.empty() || name == "." || name == "..") {
        return false;
    }

    const fs::path path(name);
    return path.filename() == path &&
           name.find_first_of("/\\") == std::string::npos;
}


std::string GetCurrentDate()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};

#ifdef _WIN32
    localtime_s(&localTime, &time);
#else
    localtime_r(&time, &localTime);
#endif

    std::ostringstream result;
    result << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return result.str();
}


bool ReadVaultMetadata(const fs::path& path, Vault& vault)
{
    try {
        std::ifstream file(path / VaultMetadataFile);
        if (!file.is_open()) {
            return false;
        }

        bool validVersion = false;
        bool validId = false;
        std::string line;

        while (std::getline(file, line)) {
            const std::size_t separator = line.find('=');
            if (separator == std::string::npos) {
                continue;
            }

            const std::string key = line.substr(0, separator);
            const std::string value = line.substr(separator + 1);

            if (key == "version") {
                validVersion = value == "1";
            }
            else if (key == "id") {
                try {
                    vault.ID = std::stoi(value);
                    validId = vault.ID > 0;
                }
                catch (...) {
                    return false;
                }
            }
            else if (key == "name") {
                vault.name = value;
            }
            else if (key == "creation_date") {
                vault.creation_date = value;
            }
        }

        vault.Path = path;
        return validVersion && validId && IsSafeVaultName(vault.name);
    }
    catch (...) {
        return false;
    }
}


bool IsValidVaultPath(const fs::path& path)
{
    try {
        return fs::is_directory(path) &&
               fs::is_regular_file(path / VaultMetadataFile) &&
               fs::is_regular_file(path / VaultDataFile);
    }
    catch (...) {
        return false;
    }
}


int GetNextVaultID()
{
    int highestID = 0;
    const fs::path root = VaultRoot();

    if (!fs::is_directory(root)) {
        return 1;
    }

    for (const auto& entry : fs::directory_iterator(root)) {
        if (!entry.is_directory()) {
            continue;
        }

        Vault vault;
        if (ReadVaultMetadata(entry.path(), vault)) {
            highestID = std::max(highestID, vault.ID);
        }
    }

    return highestID + 1;
}


void RestrictFile(const fs::path& path)
{
    fs::permissions(
        path,
        fs::perms::owner_read | fs::perms::owner_write,
        fs::perm_options::replace
    );
}


bool WriteVaultMetadata(const Vault& vault)
{
    std::ofstream file(
        vault.Path / VaultMetadataFile,
        std::ios::binary | std::ios::trunc
    );

    if (!file.is_open()) {
        return false;
    }

    file << "version=1\n"
         << "id=" << vault.ID << '\n'
         << "name=" << vault.name << '\n'
         << "creation_date=" << vault.creation_date << '\n';

    file.close();
    if (!file.good()) {
        return false;
    }

    RestrictFile(vault.Path / VaultMetadataFile);
    return true;
}


bool ReadEncryptedFile(const fs::path& path, EncryptedData& encrypted)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    std::uint8_t version = 0;
    std::uint64_t ciphertextSize = 0;

    file.read(
        reinterpret_cast<char*>(&version),
        sizeof(version)
    );
    file.read(
        reinterpret_cast<char*>(encrypted.salt.data()),
        static_cast<std::streamsize>(encrypted.salt.size())
    );
    file.read(
        reinterpret_cast<char*>(encrypted.nonce.data()),
        static_cast<std::streamsize>(encrypted.nonce.size())
    );
    file.read(
        reinterpret_cast<char*>(&ciphertextSize),
        sizeof(ciphertextSize)
    );

    if (!file.good() ||
        version != CurrentVersion ||
        ciphertextSize < crypto_aead_xchacha20poly1305_ietf_ABYTES ||
        ciphertextSize > MaximumCiphertextSize)
    {
        return false;
    }

    encrypted.version = version;
    encrypted.ciphertext.resize(
        static_cast<std::size_t>(ciphertextSize)
    );

    file.read(
        reinterpret_cast<char*>(encrypted.ciphertext.data()),
        static_cast<std::streamsize>(encrypted.ciphertext.size())
    );

    return file.good();
}


bool WriteEncryptedFile(
    const fs::path& path,
    const EncryptedData& encrypted
)
{
    const fs::path temporaryPath = path.parent_path() / PasswordTempFile;
    std::ofstream file(
        temporaryPath,
        std::ios::binary | std::ios::trunc
    );

    if (!file.is_open()) {
        return false;
    }

    const std::uint64_t ciphertextSize =
        static_cast<std::uint64_t>(encrypted.ciphertext.size());

    file.write(
        reinterpret_cast<const char*>(&encrypted.version),
        sizeof(encrypted.version)
    );
    file.write(
        reinterpret_cast<const char*>(encrypted.salt.data()),
        static_cast<std::streamsize>(encrypted.salt.size())
    );
    file.write(
        reinterpret_cast<const char*>(encrypted.nonce.data()),
        static_cast<std::streamsize>(encrypted.nonce.size())
    );
    file.write(
        reinterpret_cast<const char*>(&ciphertextSize),
        sizeof(ciphertextSize)
    );
    file.write(
        reinterpret_cast<const char*>(encrypted.ciphertext.data()),
        static_cast<std::streamsize>(encrypted.ciphertext.size())
    );
    file.close();

    if (!file.good()) {
        fs::remove(temporaryPath);
        return false;
    }

    try {
        fs::remove(path);
        fs::rename(temporaryPath, path);
        RestrictFile(path);
        return true;
    }
    catch (...) {
        fs::remove(temporaryPath);
        return false;
    }
}

}


std::string InitializeVaultDirectory()
{
    try {
        const fs::path root = VaultRoot();
        if (fs::exists(root) && !fs::is_directory(root)) {
            DirectoryExists = false;
            isFolder = false;
            return "ERROR";
        }

        fs::create_directories(root);
        DirectoryExists = fs::exists(root);
        isFolder = fs::is_directory(root);
        return DirectoryExists && isFolder ? "PASSED CHECKS" : "ERROR";
    }
    catch (...) {
        DirectoryExists = false;
        isFolder = false;
        return "ERROR";
    }
}


std::string CreateNewVault(const std::string& vaultName)
{
    if (!IsSafeVaultName(vaultName)) {
        return "INVALID_NAME";
    }

    try {
        const fs::path root = VaultRoot();
        fs::create_directories(root);
        const fs::path fullPath = root / vaultName;

        if (fs::exists(fullPath)) {
            return "VAULT_EXISTS";
        }

        if (!fs::create_directory(fullPath)) {
            return "CREATE_FAILED";
        }

        Vault vault;
        vault.name = vaultName;
        vault.ID = GetNextVaultID();
        vault.creation_date = GetCurrentDate();
        vault.Path = fullPath;

        if (!WriteVaultMetadata(vault)) {
            fs::remove_all(fullPath);
            return "METADATA_FAILED";
        }

        std::ofstream vaultData(
            fullPath / VaultDataFile,
            std::ios::binary | std::ios::trunc
        );
        vaultData << "CRYPTOR_VAULT_V1";
        vaultData.close();

        if (!vaultData.good()) {
            fs::remove_all(fullPath);
            return "VAULT_DATA_FAILED";
        }

        RestrictFile(fullPath / VaultDataFile);
        return "SUCCESS";
    }
    catch (...) {
        return "CREATE_FAILED";
    }
}


bool VaultExists(const std::string& vaultName)
{
    return IsSafeVaultName(vaultName) &&
           IsValidVaultPath(VaultRoot() / vaultName);
}


bool HasStoredPassword(const std::string& vaultName)
{
    return VaultExists(vaultName) &&
           fs::is_regular_file(VaultRoot() / vaultName / PasswordFile);
}


bool HasDecoyPassword(const std::string& vaultName)
{
    return VaultExists(vaultName) &&
           fs::is_regular_file(VaultRoot() / vaultName / DecoyPasswordFile);
}


int ConfigureDecoyPassword(
    const std::string& vaultName,
    const SecureSecret& decoyPassword
)
{
    if (decoyPassword.size() == 0 || !VaultExists(vaultName)) {
        return 1;
    }

    try {
        EncryptedData encrypted = Encrypt(
            nullptr,
            0,
            decoyPassword.data(),
            decoyPassword.size(),
            GetSecuritySettings()
        );

        return WriteEncryptedFile(
            VaultRoot() / vaultName / DecoyPasswordFile,
            encrypted
        ) ? 0 : 2;
    }
    catch (...) {
        return 3;
    }
}


int RemoveDecoyPassword(const std::string& vaultName)
{
    if (!VaultExists(vaultName)) {
        return 1;
    }

    try {
        fs::remove(VaultRoot() / vaultName / DecoyPasswordFile);
        return 0;
    }
    catch (...) {
        return 2;
    }
}


bool VerifyVault(const std::string& vaultName)
{
    if (!VaultExists(vaultName)) {
        return false;
    }

    Vault vault;
    return ReadVaultMetadata(VaultRoot() / vaultName, vault);
}


std::string LookForVaults()
{
    try {
        const fs::path root = VaultRoot();
        if (!fs::is_directory(root)) {
            return "NO_VAULTS";
        }

        bool foundVault = false;
        for (const auto& entry : fs::directory_iterator(root)) {
            if (!entry.is_directory()) {
                continue;
            }

            Vault vault;
            if (!ReadVaultMetadata(entry.path(), vault) ||
                !IsValidVaultPath(entry.path()))
            {
                continue;
            }

            foundVault = true;
            std::cout << "Vault: " << vault.name << '\n'
                      << "ID: " << vault.ID << '\n'
                      << "Created: " << vault.creation_date << '\n'
                      << "Path: " << vault.Path << "\n\n";
        }

        return foundVault ? "GOOD" : "NO_VAULTS";
    }
    catch (...) {
        return "ERROR";
    }
}


int SavePassword(
    const std::string& vaultName,
    const SecureSecret& password,
    const SecureSecret& masterPassword,
    bool decoy
)
{
    if (vaultName.empty() || password.size() == 0 || masterPassword.size() == 0) {
        return 1;
    }

    const fs::path vaultPath = VaultRoot() / vaultName;
    if (!VaultExists(vaultName)) {
        return 2;
    }

    try {
        EncryptedData encrypted = Encrypt(
            reinterpret_cast<const unsigned char*>(
                password.data()
            ),
            password.size(),
            reinterpret_cast<const unsigned char*>(
                masterPassword.data()
            ),
            masterPassword.size(),
            GetSecuritySettings()
        );

        return WriteEncryptedFile(
            vaultPath / (decoy ? DecoyPasswordFile : PasswordFile),
            encrypted
        ) ? 0 : 3;
    }
    catch (...) {
        return 4;
    }
}


int LoadPassword(
    const std::string& vaultName,
    const SecureSecret& masterPassword,
    SecureSecret& password,
    bool& decoy
)
{
    password.Clear();
    decoy = false;

    if (vaultName.empty() || masterPassword.size() == 0) {
        return 1;
    }

    const fs::path vaultPath = VaultRoot() / vaultName;
    if (!VaultExists(vaultName)) {
        return 2;
    }

    try {
        const unsigned char* passwordData = masterPassword.data();

        if (fs::is_regular_file(vaultPath / PasswordFile)) {
            try {
                EncryptedData encrypted;
                if (ReadEncryptedFile(vaultPath / PasswordFile, encrypted)) {
                    SecureSecret plaintext = Decrypt(
                        encrypted,
                        passwordData,
                        masterPassword.size(),
                        GetSecuritySettings()
                    );

                    password = std::move(plaintext);
                    return 0;
                }
            }
            catch (...) {
            }
        }

        if (fs::is_regular_file(vaultPath / DecoyPasswordFile)) {
            try {
                EncryptedData encrypted;
                if (ReadEncryptedFile(vaultPath / DecoyPasswordFile, encrypted)) {
                    SecureSecret plaintext = Decrypt(
                        encrypted,
                        passwordData,
                        masterPassword.size(),
                        GetSecuritySettings()
                    );
                    decoy = true;
                    return 1;
                }
            }
            catch (...) {
            }
        }

        decoy = true;
        return 2;
    }
    catch (...) {
        return 4;
    }
}
