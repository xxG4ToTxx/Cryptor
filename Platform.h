#pragma once
#include <cstddef>
#include <filesystem>

namespace Platform {

void ClearScreen();
bool ClearClipboard();
void AutoTypePassword(const unsigned char* password, std::size_t length);
std::filesystem::path VaultDirectory();

}