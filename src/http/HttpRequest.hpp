#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include "json.hpp"
#include "MultipartForm.hpp"
#include "../utils/CaseInsensitive.hpp"

namespace http {

enum class HttpMethod { GET, POST, PUT, PATCH, DELETE, OPTIONS, HEAD, UNKNOWN };

struct HttpRequest {
    HttpMethod method{HttpMethod::UNKNOWN};
    std::string uri;
    std::unordered_map<std::string, std::string> query;
    std::string http_version;
    std::unordered_map<std::string_view, std::string_view, utils::CaseInsensitiveHash, utils::CaseInsensitiveEqual> headers;
    std::string_view body;
    std::unordered_map<std::string, std::string> params;
    std::string client_ip;
    std::string session_id; // Set by SessionManager middleware

    nlohmann::json json() const {
        if (body.empty()) return nlohmann::json::object();
        return nlohmann::json::parse(body);
    }

    MultipartForm form() const {
        auto ct = headers.find("Content-Type");
        if (ct != headers.end() && ct->second.starts_with("multipart/form-data")) {
            return MultipartForm::parse(ct->second, body);
        }
        return {};
    }
};

} // namespace http
