#pragma once
#include <string>
#include <unordered_map>

namespace http {

enum class HttpStatus {
    OK = 200,
    BadRequest = 400,
    NotFound = 404
};

struct HttpResponse {
    HttpStatus status_code{HttpStatus::OK};
    std::unordered_map<std::string, std::string> headers;
    std::string body;

    // Helper to easily set the body and auto-fill Content-Length
    void set_body(const std::string& new_body, const std::string& content_type = "text/plain");

    // Converts the object into the raw HTTP text format for sending over TCP
    std::string serialize() const;
};

} // namespace http
