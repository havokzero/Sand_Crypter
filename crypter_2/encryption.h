#pragma once
#include <string>
#include <vector>

bool encrypt_file(const std::string& input_file, std::vector<unsigned char>& encrypted_data, const unsigned char* key, const unsigned char* iv);
bool decrypt_file(const std::vector<unsigned char>& encrypted_data, std::vector<unsigned char>& decrypted_data, const unsigned char* key, const unsigned char* iv);
void parallel_encrypt(const std::string& input_file, std::vector<unsigned char>& encrypted_data, const unsigned char* key, const unsigned char* iv);
