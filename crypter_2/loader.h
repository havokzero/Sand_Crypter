#pragma once
#include <vector>
#include <string>

bool decrypt_file(const std::vector<unsigned char>& encrypted_data, std::vector<unsigned char>& decrypted_data, const unsigned char* key, const unsigned char* iv);
