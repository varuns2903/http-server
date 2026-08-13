#pragma once
#include <orbit/http/HttpRequest.hpp>
#include <optional>
#include <string_view>

namespace http {

class HttpParser {
public:
    // Parses a raw HTTP request string into a structured HttpRequest object.
    // Returns std::nullopt if the request is malformed.
    static std::optional<HttpRequest> parse(std::string_view raw_request);

    static HttpMethod parse_method(std::string_view method_str);
};

} // namespace http
