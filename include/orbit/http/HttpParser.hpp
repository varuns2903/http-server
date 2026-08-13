#pragma once
#include <orbit/http/HttpRequest.hpp>
#include <optional>
#include <string_view>

namespace http {

/**
 * @brief Utility class for parsing HTTP requests and related components.
 */
class HttpParser {
public:
    /**
     * @brief Parses a raw HTTP request string into a structured HttpRequest object.
     * @param raw_request The raw string view of the HTTP request.
     * @return std::optional<HttpRequest> containing the parsed request, or std::nullopt if the request is malformed.
     */
    static std::optional<HttpRequest> parse(std::string_view raw_request);

    /**
     * @brief Parses an HTTP method string into an HttpMethod enum value.
     * @param method_str The string representation of the HTTP method (e.g., "GET").
     * @return The corresponding HttpMethod enum value, or HttpMethod::UNKNOWN if not recognized.
     */
    static HttpMethod parse_method(std::string_view method_str);
};

} // namespace http
