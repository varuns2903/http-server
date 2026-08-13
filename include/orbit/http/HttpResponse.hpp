#pragma once
#include <string>
#include <unordered_map>
#include <cstddef>
#include <cstdint>
#include <orbit/http/json.hpp>
#include <orbit/utils/CaseInsensitive.hpp>

namespace http {

enum class HttpStatus {
    SwitchingProtocols = 101,
    OK = 200,
    Created = 201,
    NoContent = 204,
    MovedPermanently = 301,
    Found = 302,
    NotModified = 304,
    BadRequest = 400,
    Unauthorized = 401,
    Forbidden = 403,
    NotFound = 404,
    PayloadTooLarge = 413,
    TooManyRequests = 429,
    RequestHeaderFieldsTooLarge = 431,
    UnprocessableEntity = 422,
    InternalServerError = 500
};

struct Cookie {
    std::string name;
    std::string value;
    std::string path = "/";
    std::string domain;
    long max_age = -1;
    bool secure = false;
    bool http_only = false;
    std::string same_site; // "Strict", "Lax", "None"
};

class HttpResponse {
public:
    std::vector<Cookie> cookies;
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

    HttpResponse& set_cookie(const Cookie& cookie) {
        cookies.push_back(cookie);
        return *this;
    }

    HttpResponse& set_cookie(const std::string& name, const std::string& value) {
        Cookie c;
        c.name = name;
        c.value = value;
        cookies.push_back(c);
        return *this;
    }

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
    
    // Server-Side Rendering
    void render(const std::string& template_path, const nlohmann::json& data);
    
    // Opens the file and sets up headers for sendfile()
    void send_file(const std::string& path, const std::string& content_type);

    std::string serialize() const;
    std::string serialize_headers() const;
};

} // namespace http
