#include <orbit/http/HttpParser.hpp>
#include <sstream>

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
    size_t space1 = request_line.find(' ');
    size_t space2 = request_line.find(' ', space1 + 1);
    
    if (space1 != std::string_view::npos && space2 != std::string_view::npos && space1 != space2) {
        request.method = parse_method(request_line.substr(0, space1));
        std::string full_uri = std::string(request_line.substr(space1 + 1, space2 - space1 - 1));
        
        auto q_mark = full_uri.find('?');
        if (q_mark != std::string::npos) {
            request.uri = full_uri.substr(0, q_mark);
            std::string query_string = full_uri.substr(q_mark + 1);
            
            std::istringstream q_stream(query_string);
            std::string kv;
            while (std::getline(q_stream, kv, '&')) {
                auto eq_pos = kv.find('=');
                if (eq_pos != std::string::npos) {
                    request.query[kv.substr(0, eq_pos)] = kv.substr(eq_pos + 1);
                } else {
                    request.query[kv] = ""; // Key with no value
                }
            }
        } else {
            request.uri = full_uri;
        }

        request.http_version = std::string(request_line.substr(space2 + 1));
    } else {
        return std::nullopt;
    }
    
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
