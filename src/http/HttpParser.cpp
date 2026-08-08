#include "HttpParser.hpp"

namespace http {

HttpMethod HttpParser::parse_method(std::string_view method_str) {
    if (method_str == "GET") return HttpMethod::GET;
    if (method_str == "POST") return HttpMethod::POST;
    if (method_str == "PUT") return HttpMethod::PUT;
    if (method_str == "PATCH") return HttpMethod::PATCH;
    if (method_str == "DELETE") return HttpMethod::DELETE;
    if (method_str == "OPTIONS") return HttpMethod::OPTIONS;
    if (method_str == "HEAD") return HttpMethod::HEAD;
    return HttpMethod::UNKNOWN;
}

std::optional<HttpRequest> HttpParser::parse(std::string_view raw_request) {
    HttpRequest request;
    
    // Find the end of the request line (first CRLF)
    size_t request_line_end = raw_request.find("\r\n");
    if (request_line_end == std::string_view::npos) {
        return std::nullopt; // Malformed: No CRLF found
    }
    
    std::string_view request_line = raw_request.substr(0, request_line_end);
    
    // Parse Request Line: METHOD URI VERSION
    size_t method_end = request_line.find(' ');
    if (method_end == std::string_view::npos) return std::nullopt;
    request.method = parse_method(request_line.substr(0, method_end));
    
    size_t uri_end = request_line.find(' ', method_end + 1);
    if (uri_end == std::string_view::npos) return std::nullopt;
    request.uri = request_line.substr(method_end + 1, uri_end - method_end - 1);
    
    request.version = request_line.substr(uri_end + 1);
    
    // Parse Headers
    size_t headers_start = request_line_end + 2;
    while (headers_start < raw_request.length()) {
        size_t line_end = raw_request.find("\r\n", headers_start);
        if (line_end == std::string_view::npos) return std::nullopt;
        
        // Empty line indicates end of headers
        if (line_end == headers_start) {
            headers_start += 2;
            break;
        }
        
        std::string_view header_line = raw_request.substr(headers_start, line_end - headers_start);
        size_t colon_pos = header_line.find(':');
        if (colon_pos != std::string_view::npos) {
            std::string_view key = header_line.substr(0, colon_pos);
            // Skip the colon and any following space
            size_t value_start = colon_pos + 1;
            while (value_start < header_line.length() && header_line[value_start] == ' ') {
                value_start++;
            }
            std::string_view value = header_line.substr(value_start);
            request.headers[key] = value;
        }
        
        headers_start = line_end + 2;
    }
    
    // Body is whatever is left
    if (headers_start < raw_request.length()) {
        request.body = raw_request.substr(headers_start);
    }
    
    return request;
}

} // namespace http
