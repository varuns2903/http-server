#include <orbit/middleware/JwtAuth.hpp>
#include <orbit/http/HttpResponse.hpp>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <vector>
#include <chrono>

namespace middleware {

static std::string base64url_encode(const unsigned char* input, int length) {
    int out_len = 4 * ((length + 2) / 3);
    std::vector<unsigned char> out(out_len + 1);
    EVP_EncodeBlock(out.data(), input, length);
    
    std::string b64(reinterpret_cast<char*>(out.data()));
    // Remove padding
    while (!b64.empty() && b64.back() == '=') b64.pop_back();
    // Replace characters
    for (char& c : b64) {
        if (c == '+') c = '-';
        else if (c == '/') c = '_';
    }
    return b64;
}

static std::string base64url_decode(const std::string& input) {
    std::string b64 = input;
    // Replace characters
    for (char& c : b64) {
        if (c == '-') c = '+';
        else if (c == '_') c = '/';
    }
    // Add padding
    while (b64.size() % 4 != 0) {
        b64 += '=';
    }
    
    size_t out_len = (b64.size() * 3) / 4;
    std::vector<unsigned char> out(out_len + 1);
    
    int dec_len = EVP_DecodeBlock(out.data(), reinterpret_cast<const unsigned char*>(b64.data()), b64.size());
    if (dec_len < 0) return "";
    
    int padding = 0;
    if (b64.length() > 0 && b64[b64.length() - 1] == '=') padding++;
    if (b64.length() > 1 && b64[b64.length() - 2] == '=') padding++;
    
    return std::string(reinterpret_cast<char*>(out.data()), dec_len - padding);
}

static bool verify_jwt_signature(const std::string& header_b64, const std::string& payload_b64, const std::string& provided_signature, const std::string& secret) {
    std::string data_to_sign = header_b64 + "." + payload_b64;
    
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    
    HMAC(EVP_sha256(), secret.data(), static_cast<int>(secret.length()), 
         reinterpret_cast<const unsigned char*>(data_to_sign.data()), data_to_sign.length(), 
         hash, &hash_len);
         
    std::string expected_signature = base64url_encode(hash, static_cast<int>(hash_len));
    return provided_signature == expected_signature;
}

routing::Middleware jwt_auth(const std::string& secret_key) {
    return [secret_key](http::HttpRequest& req, std::shared_ptr<http::ResponseWriter> writer) -> bool {
        auto auth_it = req.headers.find("Authorization");
        if (auth_it == req.headers.end()) {
            http::HttpResponse res;
            res.status(http::HttpStatus::Unauthorized).json(std::string(R"({"error": "Missing Authorization header"})"));
            writer->send(std::move(res));
            return false;
        }
        
        std::string_view auth_header = auth_it->second;
        if (!auth_header.starts_with("Bearer ")) {
            http::HttpResponse res;
            res.status(http::HttpStatus::Unauthorized).json(std::string(R"({"error": "Invalid Authorization scheme"})"));
            writer->send(std::move(res));
            return false;
        }
        
        std::string token(auth_header.substr(7));
        
        // Split token by '.'
        size_t first_dot = token.find('.');
        size_t second_dot = token.find('.', first_dot + 1);
        
        if (first_dot == std::string::npos || second_dot == std::string::npos) {
            http::HttpResponse res;
            res.status(http::HttpStatus::Unauthorized).json(std::string(R"({"error": "Malformed JWT"})"));
            writer->send(std::move(res));
            return false;
        }
        
        std::string header_b64 = token.substr(0, first_dot);
        std::string payload_b64 = token.substr(first_dot + 1, second_dot - first_dot - 1);
        std::string signature_b64 = token.substr(second_dot + 1);
        
        if (!verify_jwt_signature(header_b64, payload_b64, signature_b64, secret_key)) {
            http::HttpResponse res;
            res.status(http::HttpStatus::Unauthorized).json(std::string(R"({"error": "Invalid JWT signature"})"));
            writer->send(std::move(res));
            return false;
        }
        
        std::string payload_json_str = base64url_decode(payload_b64);
        if (payload_json_str.empty()) {
            http::HttpResponse res;
            res.status(http::HttpStatus::Unauthorized).json(std::string(R"({"error": "Invalid payload encoding"})"));
            writer->send(std::move(res));
            return false;
        }
        
        try {
            req.user = nlohmann::json::parse(payload_json_str);
        } catch (...) {
            http::HttpResponse res;
            res.status(http::HttpStatus::Unauthorized).json(std::string(R"({"error": "Payload is not valid JSON"})"));
            writer->send(std::move(res));
            return false;
        }
        
        // Check expiration
        if (req.user.contains("exp") && req.user["exp"].is_number()) {
            long long exp = req.user["exp"].get<long long>();
            auto now = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count();
            if (now > exp) {
                http::HttpResponse res;
                res.status(http::HttpStatus::Unauthorized).json(std::string(R"({"error": "Token expired"})"));
                writer->send(std::move(res));
                return false;
            }
        }
        
        return true;
    };
}

} // namespace middleware
