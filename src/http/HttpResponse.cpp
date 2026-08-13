#include <orbit/http/HttpResponse.hpp>
#ifdef _WIN32
#include <io.h>
#define open _open
#define close _close
#endif
#include <sstream>
#include <fcntl.h>
#include <sys/stat.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <inja/inja.hpp>

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

void HttpResponse::render(const std::string& template_path, const nlohmann::json& data) {
    try {
        inja::Environment env;
        std::string result = env.render_file(template_path, data);
        set_body(result, "text/html");
    } catch (const std::exception& e) {
        status_code = HttpStatus::InternalServerError;
        set_body(std::string("<h1>500 Internal Server Error</h1><p>Template Error: ") + e.what() + "</p>", "text/html");
    }
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
        case HttpStatus::SwitchingProtocols: oss << "101 Switching Protocols\r\n"; break;
        case HttpStatus::OK: oss << "200 OK\r\n"; break;
        case HttpStatus::Created: oss << "201 Created\r\n"; break;
        case HttpStatus::NoContent: oss << "204 No Content\r\n"; break;
        case HttpStatus::NotModified: oss << "304 Not Modified\r\n"; break;
        case HttpStatus::BadRequest: oss << "400 Bad Request\r\n"; break;
        case HttpStatus::Unauthorized: oss << "401 Unauthorized\r\n"; break;
        case HttpStatus::Forbidden: oss << "403 Forbidden\r\n"; break;
        case HttpStatus::NotFound: oss << "404 Not Found\r\n"; break;
        case HttpStatus::PayloadTooLarge: oss << "413 Payload Too Large\r\n"; break;
        case HttpStatus::TooManyRequests: oss << "429 Too Many Requests\r\n"; break;
        case HttpStatus::RequestHeaderFieldsTooLarge: oss << "431 Request Header Fields Too Large\r\n"; break;
        case HttpStatus::UnprocessableEntity: oss << "422 Unprocessable Entity\r\n"; break;
        case HttpStatus::InternalServerError: oss << "500 Internal Server Error\r\n"; break;
    }

    bool has_content_length = false;
    for (const auto& [key, value] : headers) {
        if (key == "Content-Length") has_content_length = true;
        oss << key << ": " << value << "\r\n";
    }

    if (!has_content_length && !body.empty()) {
        oss << "Content-Length: " << body.length() << "\r\n";
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
