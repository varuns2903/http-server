#pragma once
#include <string>
#include <openssl/sha.h>
#include <openssl/evp.h>

namespace http {
namespace websocket {

class Handshake {
public:
    static std::string generate_accept_key(const std::string& client_key) {
        const std::string magic_string = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        std::string combined = client_key + magic_string;
        
        unsigned char hash[SHA_DIGEST_LENGTH];
        SHA1(reinterpret_cast<const unsigned char*>(combined.c_str()), combined.length(), hash);
        
        // Base64 encode using OpenSSL EVP
        int expected_len = 4 * ((SHA_DIGEST_LENGTH + 2) / 3);
        std::string base64_key(static_cast<size_t>(expected_len), '\0');
        
        int encoded_len = EVP_EncodeBlock(
            reinterpret_cast<unsigned char*>(&base64_key[0]),
            hash,
            SHA_DIGEST_LENGTH
        );
        
        base64_key.resize(static_cast<size_t>(encoded_len));
        return base64_key;
    }
};

} // namespace websocket
} // namespace http
