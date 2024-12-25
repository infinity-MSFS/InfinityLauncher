
#include "Encryption.hpp"

#include <ranges>

namespace Infinity {

    using namespace Errors;

    std::unordered_map<Keys, ByteVector> Key::GetKeyMap() {
        std::unordered_map<Keys, ByteVector> keyMap;
        keyMap.insert({Keys::Client, CreateUnencryptedKey(client_key)});
        return keyMap;
    }

    ByteVector Encryption::Encrypt(const std::string &plain_text, const ByteVector &key) {
        if (key.size() != 32) {
            Error(ErrorType::Fatal, "Invalid Key Size").Dispatch();
        }

        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            Error(ErrorType::Fatal, "Failed to create encryption cipher context").Dispatch();
        }
        ByteVector iv = GenerateIV();

        if (!EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data())) {
            EVP_CIPHER_CTX_free(ctx);
            Error(ErrorType::Fatal, "Failed to initialize encryption").Dispatch();
        }

        ByteVector ciphertext(plain_text.size() + EVP_MAX_BLOCK_LENGTH + EVP_MAX_IV_LENGTH);

        std::ranges::copy(iv, ciphertext.begin());
        int offset = EVP_MAX_IV_LENGTH;
        int len;
        if (!EVP_EncryptUpdate(ctx, ciphertext.data() + offset, &len, reinterpret_cast<const unsigned char *>(plain_text.c_str()), static_cast<int>(plain_text.size()))) {
            EVP_CIPHER_CTX_free(ctx);
            Error(ErrorType::Fatal, "Failed encrypt data").Dispatch();
        }
        offset += len;

        if (!EVP_EncryptFinal_ex(ctx, ciphertext.data() + offset, &len)) {
            EVP_CIPHER_CTX_free(ctx);
            Error(ErrorType::Fatal, "Failed finalize encryption").Dispatch();
        }
        offset += len;

        EVP_CIPHER_CTX_free(ctx);

        ciphertext.resize(offset);
        return ciphertext;
    }

    ByteVector Encryption::Encrypt(const ByteVector &binary, const ByteVector &key) {
        if (key.size() != 32) {
            Error(ErrorType::Fatal, "Attempted encryption with invalid key").Dispatch();
        }
        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            Error(ErrorType::Fatal, "Failed to create encryption cipher context").Dispatch();
        }

        ByteVector iv = GenerateIV();

        if (!EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data())) {
            EVP_CIPHER_CTX_free(ctx);
            Error(ErrorType::Fatal, "Failed to initialize encryption").Dispatch();
        }

        ByteVector ciphertext(binary.size() + EVP_MAX_BLOCK_LENGTH + EVP_MAX_IV_LENGTH);

        std::ranges::copy(iv, ciphertext.begin());
        int offset = EVP_MAX_IV_LENGTH;

        int len;
        if (!EVP_EncryptUpdate(ctx, ciphertext.data() + offset, &len, binary.data(), static_cast<int>(binary.size()))) {
            EVP_CIPHER_CTX_free(ctx);
            Error(ErrorType::Fatal, "Failed encrypt data").Dispatch();
        }

        offset += len;

        if (!EVP_EncryptFinal_ex(ctx, ciphertext.data() + offset, &len)) {
            EVP_CIPHER_CTX_free(ctx);
            Error(ErrorType::Fatal, "Failed finalize encryption").Dispatch();
        }
        offset += len;

        EVP_CIPHER_CTX_free(ctx);

        ciphertext.resize(offset);
        return ciphertext;
    }


    std::string Encryption::EncryptBase64(const std::string &plain_text, const ByteVector &key) {
        const auto bytes = Encrypt(plain_text, key);
        return Base64Encode(bytes);
    }

    std::string Encryption::EncryptBase64(const ByteVector &binary, const ByteVector &key) {
        const auto bytes = Encrypt(binary, key);
        return Base64Encode(bytes);
    }

    std::string Encryption::Decrypt(const ByteVector &binary, const ByteVector &key) {
        if (key.size() != 32) {
            Error(ErrorType::Fatal, "Attempted decryption with invalid key").Dispatch();
        }

        if (binary.size() <= EVP_MAX_IV_LENGTH) {
            Error(ErrorType::Fatal, "Binary too short").Dispatch();
        }

        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            Error(ErrorType::Fatal, "Failed to create cipher context").Dispatch();
        }

        const std::vector iv(binary.begin(), binary.begin() + EVP_MAX_IV_LENGTH);

        if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data())) {
            EVP_CIPHER_CTX_free(ctx);
            Error(ErrorType::Fatal, "Failed to initialize decryption").Dispatch();
        }


        ByteVector plain_text(binary.size() - EVP_MAX_IV_LENGTH);
        int len;

        if (!EVP_DecryptUpdate(ctx, plain_text.data(), &len, binary.data() + EVP_MAX_IV_LENGTH, static_cast<int>(binary.size()) - EVP_MAX_IV_LENGTH)) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to decrypt data");
        }
        int plaintext_len = len;

        if (!EVP_DecryptFinal_ex(ctx, plain_text.data() + len, &len)) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to finalize decryption");
        }
        plaintext_len += len;

        EVP_CIPHER_CTX_free(ctx);

        return {plain_text.begin(), plain_text.begin() + plaintext_len};
    }


    ByteVector Encryption::DecryptToBin(const ByteVector &binary, const ByteVector &key) {

        if (key.size() != 32) {
            Error(ErrorType::Fatal, "Attempted decryption with invalid key").Dispatch();
        }

        if (binary.size() < EVP_MAX_IV_LENGTH) {
            Error(ErrorType::Fatal, "Binary too short to extract IV").Dispatch();
        }


        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            Error(ErrorType::Fatal, "Failed to create cipher context").Dispatch();
        }

        const std::vector iv(binary.begin(), binary.begin() + EVP_MAX_IV_LENGTH);

        if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data())) {
            EVP_CIPHER_CTX_free(ctx);
            Error(ErrorType::Fatal, "Failed to initialize decryption").Dispatch();
        }

        std::vector<uint8_t> plaintext(binary.size() - EVP_MAX_IV_LENGTH);
        int len;

        if (!EVP_DecryptUpdate(ctx, plaintext.data(), &len, binary.data() + EVP_MAX_IV_LENGTH, static_cast<int>(binary.size() - EVP_MAX_IV_LENGTH))) {
            EVP_CIPHER_CTX_free(ctx);
            Error(ErrorType::Fatal, "Failed to decrypt data").Dispatch();
        }
        int plaintext_len = len;

        // if (!EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len)) {
        //     EVP_CIPHER_CTX_free(ctx);
        //     Error(ErrorType::Fatal, "Failed to finalize decryption").Dispatch();
        // }
        // plaintext_len += len;

        EVP_CIPHER_CTX_free(ctx);

        plaintext.resize(plaintext_len);
        return plaintext;
    }

    std::string Encryption::DecryptBase64(const std::string &base64, const ByteVector &key) {
        const auto bytes = Base64Decode(base64);
        return Decrypt(bytes, key);
    }

    ByteVector Encryption::DecryptBase64Bin(const std::string &base64, const ByteVector &key) {
        const auto bytes = Base64Decode(base64);
        return DecryptToBin(bytes, key);
    }

    ByteVector Encryption::GenerateIV() {
        ByteVector IV(EVP_MAX_IV_LENGTH);
        if (!RAND_bytes(IV.data(), EVP_MAX_IV_LENGTH)) {
            Error(ErrorType::Fatal, "Failed to generate IV").Dispatch();
        }
        return IV;
    }

    std::string Encryption::Base64Encode(const ByteVector &binary) {
        BUF_MEM *buffer_ptr;

        BIO *b64 = BIO_new(BIO_f_base64());
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
        BIO *bio = BIO_new(BIO_s_mem());
        bio = BIO_push(b64, bio);

        BIO_write(bio, binary.data(), static_cast<int>(binary.size()));
        BIO_flush(bio);
        BIO_get_mem_ptr(bio, &buffer_ptr);

        std::string result(buffer_ptr->data, buffer_ptr->length);

        BIO_free_all(bio);

        return result;
    }

    ByteVector Encryption::Base64Decode(const std::string &base64_data) {
        ByteVector result(base64_data.size());

        BIO *base64 = BIO_new(BIO_f_base64());
        BIO_set_flags(base64, BIO_FLAGS_BASE64_NO_NL);
        BIO *bio = BIO_new_mem_buf(base64_data.data(), -1);
        bio = BIO_push(base64, bio);

        const int decoded_size = BIO_read(bio, result.data(), static_cast<int>(base64_data.size()));
        BIO_free_all(bio);
        if (decoded_size < 0) {
            Error(ErrorType::Fatal, "Failed to decode base64 data").Dispatch();
        }

        result.resize(decoded_size);
        return result;
    }

}
