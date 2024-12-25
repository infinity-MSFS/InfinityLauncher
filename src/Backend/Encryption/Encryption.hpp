
#pragma once

#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "Util/Error/Error.hpp"
#include "openssl/aes.h"
#include  "openssl/bio.h"
#include "openssl/buffer.h"
#include "openssl/evp.h"
#include "openssl/rand.h"

#include "client_key.h"

#ifdef WIN32
#include <Windows.h>
#include <iostream>
#endif

namespace Infinity {
    typedef std::vector<uint8_t> ByteVector;


    enum class Keys { Client, Unknown };

    struct Key {
        ByteVector key;
        Keys name;

        static std::unordered_map<Keys, ByteVector> GetKeyMap();
    };

    class Encryption {
    public:
        // The launcher doesnt use any encryption currently
        [[maybe_unused]] static ByteVector Encrypt(const std::string &plain_text, const ByteVector &key);
        [[maybe_unused]] static ByteVector Encrypt(const ByteVector &binary, const ByteVector &key);
        [[maybe_unused]] static std::string EncryptBase64(const std::string &plain_text, const ByteVector &key);
        [[maybe_unused]] static std::string EncryptBase64(const ByteVector &binary, const ByteVector &key);

        static std::string Decrypt(const ByteVector &binary, const ByteVector &key);
        static ByteVector DecryptToBin(const ByteVector &binary, const ByteVector &key);
        static std::string DecryptBase64(const std::string &binary, const ByteVector &key);
        static ByteVector DecryptBase64Bin(const std::string &binary, const ByteVector &key);

        static std::string Base64Encode(const ByteVector &binary);
        static ByteVector Base64Decode(const std::string &b64);

    private:
        static ByteVector GenerateIV();

    };

    inline ByteVector CreateUnencryptedKey(const unsigned char *raw_bin) {
        ByteVector key(raw_bin, raw_bin + sizeof(raw_bin));
        return key;
    }

    inline ByteVector CreateEncryptedKey(const unsigned char *raw_bin, const ByteVector &key) {
        const ByteVector encrypted_key(raw_bin, raw_bin + sizeof(raw_bin));
        auto result = Encryption::DecryptToBin(encrypted_key, key);
        return result;
    }
}
