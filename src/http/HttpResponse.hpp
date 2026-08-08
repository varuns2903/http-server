#pragma once
#include <string>
#include <unordered_map>
#include <unistd.h>
#include <sys/types.h>

namespace http {

enum class HttpStatus {
    OK = 200,
    BadRequest = 400,
    NotFound = 404,
    InternalServerError = 500
};

class HttpResponse {
public:
    HttpStatus status_code = HttpStatus::OK;
    std::unordered_map<std::string, std::string> headers;
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
    
    // Opens the file and sets up headers for sendfile()
    void send_file(const std::string& path, const std::string& content_type);

    std::string serialize() const;
    std::string serialize_headers() const;
};

} // namespace http
