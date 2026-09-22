#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <cstdint>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include "Encryption.h"

namespace fs = std::filesystem;

bool DirectoryExists = false;
bool isFolder = false;

namespace {

constexpr const char* VaultRoot =
    "C:/Documents/cryptor";

constexpr const char* VaultMetadataFile =
    "vault.meta";

constexpr const char* VaultDataFile =
    "vault.cryptor";


bool ReadVaultMetadata(
    const fs::path& path,
    struct Vault& vault
);

bool IsValidVault(
    const fs::path& path
)
{
    if (!fs::exists(path)) {
        return false;
    }

    if (!fs::is_directory(path)) {
        return false;
    }

    if (!fs::exists(
            path / VaultMetadataFile
        ))
    {
        return false;
    }

    if (!fs::exists(
            path / VaultDataFile
        ))
    {
        return false;
    }

    return true;
}


std::string GetCurrentDate()
{
    const auto now =
        std::chrono::system_clock::now();

    const std::time_t time =
        std::chrono::system_clock::to_time_t(now);

    std::tm localTime{};

#ifdef _WIN32
    localtime_s(
        &localTime,
        &time
    );
#else
    localtime_r(
        &time,
        &localTime
    );
#endif

    std::ostringstream result;

    result
        << std::put_time(
            &localTime,
            "%Y-%m-%d %H:%M:%S"
        );

    return result.str();
}


bool WriteVaultMetadata(
    const struct Vault& vault
)
{
    try {

        fs::path path =
            fs::path(vault.Path) /
            VaultMetadataFile;

        std::ofstream file(
            path,
            std::ios::binary |
            std::ios::trunc
        );

        if (!file.is_open()) {
            return false;
        }

        file
            << "version=1\n"
            << "id=" << vault.ID << '\n'
            << "name=" << vault.name << '\n'
            << "creation_date="
            << vault.creation_date
            << '\n';

        return file.good();
    }
    catch (...) {
        return false;
    }
}


bool ReadVaultMetadata(
    const fs::path& path,
    struct Vault& vault
)
{
    try {

        std::ifstream file(
            path / VaultMetadataFile
        );

        if (!file.is_open()) {
            return false;
        }

        std::string line;

        while (std::getline(file, line)) {

            const std::size_t separator =
                line.find('=');

            if (separator == std::string::npos) {
                continue;
            }

            const std::string key =
                line.substr(
                    0,
                    separator
                );

            const std::string value =
                line.substr(
                    separator + 1
                );

            if (key == "id") {

                try {
                    vault.ID =
                        std::stoi(value);
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

        vault.Path =
            path.string();

        return !vault.name.empty();
    }
    catch (...) {
        return false;
    }
}


int GetNextVaultID()
{
    int highestID = 0;

    const fs::path root =
        VaultRoot;

    if (!fs::exists(root)) {
        return 1;
    }

    for (const auto& entry :
         fs::directory_iterator(root))
    {
        if (!entry.is_directory()) {
            continue;
        }

        Vault vault{};

        if (!ReadVaultMetadata(
                entry.path(),
                vault
            ))
        {
            continue;
        }

        highestID =
            std::max(
                highestID,
                vault.ID
            );
    }

    return highestID + 1;
}

}


struct Vault {
    std::string name;
    int ID;
    std::string creation_date;
    std::string Path;
};


std::string CreateNewVault(
    std::string NextTask
)
{
    std::string VaultName;

    system("cls");

    std::cout
        << "enter desired name of vault\n"
        << "  > ";

    std::cin >> VaultName;

    if (VaultName.empty()) {
        return "INVALID_NAME";
    }

    fs::path targetFolder =
        VaultRoot;

    try {

        fs::create_directories(
            targetFolder
        );

        fs::path fullPath =
            targetFolder / VaultName;

        if (fs::exists(fullPath)) {
            return "VAULT_EXISTS";
        }

        if (!fs::create_directory(fullPath)) {
            return "CREATE_FAILED";
        }

        Vault vault;

        vault.name =
            VaultName;

        vault.ID =
            GetNextVaultID();

        vault.creation_date =
            GetCurrentDate();

        vault.Path =
            fullPath.string();

        if (!WriteVaultMetadata(vault)) {
            fs::remove_all(fullPath);

            return "METADATA_FAILED";
        }

        std::ofstream vaultData(
            fullPath / VaultDataFile,
            std::ios::binary |
            std::ios::trunc
        );

        if (!vaultData.is_open()) {
            fs::remove_all(fullPath);

            return "VAULT_DATA_FAILED";
        }

        vaultData
            << "CRYPTOR_VAULT_V1";

        vaultData.close();

        if (!vaultData.good()) {
            fs::remove_all(fullPath);

            return "VAULT_DATA_FAILED";
        }

        return "SUCCESS";
    }
    catch (...) {
        return "CREATE_FAILED";
    }
}


std::string LookForVaults()
{
    const fs::path targetFolder =
        VaultRoot;

    try {

        if (!fs::exists(targetFolder)) {
            return "NO_VAULTS";
        }

        if (!fs::is_directory(targetFolder)) {
            return "INVALID_ROOT";
        }

        bool foundVault = false;

        for (const auto& entry :
             fs::directory_iterator(targetFolder))
        {
            if (!entry.is_directory()) {
                continue;
            }

            Vault vault{};

            if (!ReadVaultMetadata(
                    entry.path(),
                    vault
                ))
            {
                continue;
            }

            if (!IsValidVault(
                    entry.path()
                ))
            {
                continue;
            }

            foundVault = true;

            std::cout
                << "\nVault: "
                << vault.name
                << '\n';

            std::cout
                << "ID: "
                << vault.ID
                << '\n';

            std::cout
                << "Created: "
                << vault.creation_date
                << '\n';

            std::cout
                << "Path: "
                << vault.Path
                << '\n';
        }

        if (!foundVault) {
            return "NO_VAULTS";
        }

        return "GOOD";
    }
    catch (...) {
        return "ERROR";
    }
}


std::string InitializeVaultDirectory()
{
    fs::path targetFolder =
        VaultRoot;

    try {

        if (!fs::exists(
                targetFolder
            ))
        {
            fs::create_directories(
                targetFolder
            );

            DirectoryExists = true;
            isFolder = true;
        }
        else if (!fs::is_directory(
                     targetFolder
                 ))
        {
            std::cerr
                << "Fatal Error: folder location is a file...\n"
                << " <> try deleting the file or restarting "
                << "the application";

            DirectoryExists = false;
            isFolder = false;
        }
        else {

            isFolder = true;
            DirectoryExists = true;
        }

        if (
            DirectoryExists &&
            isFolder
        ) {
            return "PASSED CHECKS";
        }

        if (
            DirectoryExists &&
            !isFolder
        ) {
            return "ERROR";
        }
    }
    catch (...) {
        return "ERROR";
    }

    return "UNEXPECTED";
}


int SavePassword(
    std::string VaultName,
    std::string Password,
    std::string MasterPassword
)
{
    if (VaultName.empty()) {
        return 1;
    }

    if (Password.empty()) {
        return 2;
    }

    if (MasterPassword.empty()) {
        return 3;
    }

    const fs::path vaultPath =
        fs::path(VaultRoot) /
        VaultName;

    try {

        if (!IsValidVault(vaultPath)) {
            return 4;
        }

        SecuritySettings securitySettings;

        EncryptedData encrypted =
            Encrypt(
                reinterpret_cast<
                    const unsigned char*
                >(
                    Password.data()
                ),
                Password.size(),
                reinterpret_cast<
                    const unsigned char*
                >(
                    MasterPassword.data()
                ),
                MasterPassword.size(),
                securitySettings
            );

        fs::path passwordFile =
            vaultPath / "password.cryptor";

        std::ofstream file(
            passwordFile,
            std::ios::binary |
            std::ios::trunc
        );

        if (!file.is_open()) {
            return 5;
        }

        file.write(
            reinterpret_cast<
                const char*
            >(
                &encrypted.version
            ),
            sizeof(
                encrypted.version
            )
        );

        file.write(
            reinterpret_cast<
                const char*
            >(
                encrypted.salt.data()
            ),
            static_cast<
                std::streamsize
            >(
                encrypted.salt.size()
            )
        );

        file.write(
            reinterpret_cast<
                const char*
            >(
                encrypted.nonce.data()
            ),
            static_cast<
                std::streamsize
            >(
                encrypted.nonce.size()
            )
        );

        const std::uint64_t ciphertextSize =
            static_cast<
                std::uint64_t
            >(
                encrypted.ciphertext.size()
            );

        file.write(
            reinterpret_cast<
                const char*
            >(
                &ciphertextSize
            ),
            sizeof(ciphertextSize)
        );

        if (!encrypted.ciphertext.empty()) {

            file.write(
                reinterpret_cast<
                    const char*
                >(
                    encrypted.ciphertext.data()
                ),
                static_cast<
                    std::streamsize
                >(
                    encrypted.ciphertext.size()
                )
            );
        }

        file.flush();

        if (!file.good()) {
            file.close();

            return 6;
        }

        file.close();

        return 0;
    }
    catch (...) {
        return 7;
    }
}
