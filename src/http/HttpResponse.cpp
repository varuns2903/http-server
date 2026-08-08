#include "HttpResponse.hpp"
#include <sstream>

namespace http {

std::string status_to_string(HttpStatus status) {
    switch (status) {
        case HttpStatus::OK: return "200 OK";
        case HttpStatus::BadRequest: return "400 Bad Request";
        case HttpStatus::NotFound: return "404 Not Found";
        default: return "500 Internal Server Error";
    }
}

void HttpResponse::set_body(const std::string& new_body, const std::string& content_type) {
    body = new_body;
    headers["Content-Length"] = std::to_string(body.length());
    headers["Content-Type"] = content_type;
}

std::string HttpResponse::serialize() const {
    std::ostringstream out;
    // 1. Response Line
    out << "HTTP/1.1 " << status_to_string(status_code) << "\r\n";
    
    // 2. Headers
    for (const auto& [key, value] : headers) {
        out << key << ": " << value << "\r\n";
    }
    
    // 3. Blank Line
    out << "\r\n";
    
    // 4. Body
    out << body;
    
    return out.str();
}

} // namespace http
