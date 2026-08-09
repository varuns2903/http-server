#pragma once
#include <string>
#include <unordered_map>
#include <unistd.h>
#include <sys/types.h>
#include "json.hpp"
#include "../utils/CaseInsensitive.hpp"

namespace http {

enum class HttpStatus {
    SwitchingProtocols = 101,
    OK = 200,
    Created = 201,
    NoContent = 204,
    BadRequest = 400,
    Forbidden = 403,
    NotFound = 404,
    PayloadTooLarge = 413,
    TooManyRequests = 429,
    RequestHeaderFieldsTooLarge = 431,
    InternalServerError = 500
};

class HttpResponse {
public:
    HttpStatus status_code = HttpStatus::OK;
    std::unordered_map<std::string, std::string, utils::CaseInsensitiveHash, utils::CaseInsensitiveEqual> headers;
    std::string body;
    
    // Zero-copy file descriptors
    int file_fd{-1};
    off_t file_size{0};

    HttpResponse() = default;

    // Move semantics to manage the file_fd lifecycle safely
    HttpResponse(HttpResponse&& other) noexcept;
    HttpResponse& operator=(HttpResponse&& other) noexcept;
    ~HttpResponse();

    // Disable copy to prevent double-closing FDs
    HttpResponse(const HttpResponse&) = delete;
    HttpResponse& operator=(const HttpResponse&) = delete;

    void set_body(const std::string& b, const std::string& content_type = "text/plain");
    
    // Ergonomic fluent helpers
    HttpResponse& status(HttpStatus code) {
        status_code = code;
        return *this;
    }
    void send(const std::string& b) { set_body(b, "text/plain"); }
    void json(const std::string& j) { set_body(j, "application/json"); }
    void json(const nlohmann::json& j) { set_body(j.dump(), "application/json"); }
    void html(const std::string& h) { set_body(h, "text/html"); }
    
    // Opens the file and sets up headers for sendfile()
    void send_file(const std::string& path, const std::string& content_type);

    std::string serialize() const;
    std::string serialize_headers() const;
};

} // namespace http
