#include "pch.h"
#include "loader.h"
#include <windows.h>
#include <vector>
#include <string>
#include <fstream>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <shellapi.h>

bool decrypt_file(const std::vector<unsigned char>& encrypted_data, std::vector<unsigned char>& decrypted_data, const unsigned char* key, const unsigned char* iv) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return false;

    int len;
    int plaintext_len;
    std::vector<unsigned char> plaintext(encrypted_data.size());

    if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    if (!EVP_DecryptUpdate(ctx, plaintext.data(), &len, encrypted_data.data(), static_cast<int>(encrypted_data.size()))) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    plaintext_len = len;

    if (!EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len)) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    plaintext.resize(plaintext_len);
    decrypted_data = std::move(plaintext);

    return true;
}
