#include "HttpResponse.hpp"
#include <sstream>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace http {

HttpResponse::HttpResponse(HttpResponse&& other) noexcept 
    : status_code(other.status_code),
      headers(std::move(other.headers)),
      body(std::move(other.body)),
      file_fd(other.file_fd),
      file_size(other.file_size) {
    other.file_fd = -1; // Steal ownership
}

HttpResponse& HttpResponse::operator=(HttpResponse&& other) noexcept {
    if (this != &other) {
        if (file_fd != -1) close(file_fd);
        status_code = other.status_code;
        headers = std::move(other.headers);
        body = std::move(other.body);
        file_fd = other.file_fd;
        file_size = other.file_size;
        other.file_fd = -1;
    }
    return *this;
}

HttpResponse::~HttpResponse() {
    if (file_fd != -1) {
        close(file_fd);
    }
}

void HttpResponse::set_body(const std::string& b, const std::string& content_type) {
    body = b;
    headers["Content-Type"] = content_type;
    headers["Content-Length"] = std::to_string(body.length());
}

void HttpResponse::send_file(const std::string& path, const std::string& content_type) {
    if (file_fd != -1) {
        close(file_fd);
    }
    
    file_fd = open(path.c_str(), O_RDONLY);
    if (file_fd == -1) {
        status_code = HttpStatus::NotFound;
        set_body("<h1>404 Not Found</h1>", "text/html");
        return;
    }
    
    struct stat stat_buf;
    if (fstat(file_fd, &stat_buf) == 0) {
        file_size = stat_buf.st_size;
        headers["Content-Length"] = std::to_string(file_size);
        headers["Content-Type"] = content_type;
    } else {
        close(file_fd);
        file_fd = -1;
        status_code = HttpStatus::InternalServerError;
        set_body("<h1>500 Internal Error</h1>", "text/html");
    }
}

std::string HttpResponse::serialize_headers() const {
    std::ostringstream oss;
    oss << "HTTP/1.1 ";
    switch (status_code) {
        case HttpStatus::OK: oss << "200 OK\r\n"; break;
        case HttpStatus::BadRequest: oss << "400 Bad Request\r\n"; break;
        case HttpStatus::NotFound: oss << "404 Not Found\r\n"; break;
        case HttpStatus::InternalServerError: oss << "500 Internal Server Error\r\n"; break;
    }

    for (const auto& [key, value] : headers) {
        oss << key << ": " << value << "\r\n";
    }
    oss << "\r\n";
    return oss.str();
}

std::string HttpResponse::serialize() const {
    if (file_fd != -1) {
        return serialize_headers();
    }
    return serialize_headers() + body;
}

} // namespace http
