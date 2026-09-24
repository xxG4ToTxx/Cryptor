#pragma once

#include <filesystem>

namespace Platform {

void ClearScreen();
bool ClearClipboard();
std::filesystem::path VaultDirectory();

}