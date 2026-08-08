#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include "json.hpp"

namespace http {

enum class HttpMethod { GET, POST, PUT, PATCH, DELETE, OPTIONS, HEAD, UNKNOWN };

struct HttpRequest {
    HttpMethod method{HttpMethod::UNKNOWN};
    std::string_view uri;
    std::string_view version;
    std::unordered_map<std::string_view, std::string_view> headers;
    std::string_view body;
    std::unordered_map<std::string, std::string> params;

    nlohmann::json json() const {
        if (body.empty()) return nlohmann::json::object();
        return nlohmann::json::parse(body);
    }
};

} // namespace http
