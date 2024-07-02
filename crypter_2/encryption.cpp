#include "pch.h"
#include "encryption.h"
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <fstream>
#include <vector>
#include <thread>
#include <future>
#include <iostream>

bool encrypt_file(const std::string& input_file, std::vector<unsigned char>& encrypted_data, const unsigned char* key, const unsigned char* iv) {
    std::ifstream ifs(input_file, std::ios::binary);
    if (!ifs.is_open()) {
        return false;
    }

    std::vector<unsigned char> plaintext((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return false;

    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    int len;
    int ciphertext_len;
    std::vector<unsigned char> ciphertext(plaintext.size() + EVP_CIPHER_block_size(EVP_aes_256_cbc()));

    if (1 != EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext.data(), static_cast<int>(plaintext.size()))) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    ciphertext_len = len;

    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len)) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    ciphertext.resize(ciphertext_len);
    encrypted_data = std::move(ciphertext);

    return true;
}

void parallel_encrypt(const std::string& input_file, std::vector<unsigned char>& encrypted_data, const unsigned char* key, const unsigned char* iv) {
    std::thread encrypt_thread([=, &encrypted_data] {
        encrypt_file(input_file, encrypted_data, key, iv);
        });
    encrypt_thread.join();
}
