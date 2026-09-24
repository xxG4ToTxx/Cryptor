
  
  #pragma once

  #include <string>
  #include <vector>

  #include "Encryption.h"


std::string InitializeVaultDirectory();
std::string LookForVaults();
  std::string CreateNewVault(const std::string& vaultName);
  bool VaultExists(const std::string& vaultName);
  bool HasStoredPassword(const std::string& vaultName);
  bool HasDecoyPassword(const std::string& vaultName);
  bool VerifyVault(const std::string& vaultName);

  int ConfigureDecoyPassword(
    const std::string& vaultName,
    const SecureSecret& decoyPassword
  );

  int RemoveDecoyPassword(const std::string& vaultName);

  int SavePassword(
    const std::string& vaultName,
    const SecureSecret& password,
    const SecureSecret& masterPassword,
    bool decoy
  );

  int LoadPassword(
    const std::string& vaultName,
    const SecureSecret& masterPassword,
    SecureSecret& password,
    bool& decoy
  );
  
