#pragma once
#include <string>
#include <string_view>
#include <unordered_map>

namespace http {

enum class HttpMethod { GET, POST, HEAD, UNKNOWN };

struct HttpRequest {
    HttpMethod method{HttpMethod::UNKNOWN};
    std::string_view uri;
    std::string_view version;
    std::unordered_map<std::string_view, std::string_view> headers;
    std::string_view body;
};

} // namespace http
