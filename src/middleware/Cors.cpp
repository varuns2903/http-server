#include "Cors.hpp"
#include <sstream>

namespace middleware {

namespace {
    std::string join(const std::vector<std::string>& vec, const std::string& delimiter) {
        if (vec.empty()) return "";
        std::ostringstream os;
        for (size_t i = 0; i < vec.size(); ++i) {
            os << vec[i];
            if (i != vec.size() - 1) os << delimiter;
        }
        return os.str();
    }
}

routing::Middleware cors(CorsOptions options) {
    std::string methods_str = join(options.allowed_methods, ", ");
    std::string headers_str = join(options.allowed_headers, ", ");
    
    return [options, methods_str, headers_str](http::HttpRequest& request, http::HttpResponse& response) -> bool {
        
        // 1. Calculate Origin
        std::string origin = "*";
        if (options.allowed_origins.size() == 1 && options.allowed_origins[0] == "*") {
            origin = "*";
        } else {
            auto it = request.headers.find("Origin");
            if (it != request.headers.end()) {
                std::string req_origin(it->second);
                for (const auto& allowed : options.allowed_origins) {
                    if (allowed == req_origin) {
                        origin = req_origin;
                        break;
                    }
                }
            }
        }
        
        // 2. Attach standard headers to every response
        response.headers["Access-Control-Allow-Origin"] = origin;
        if (options.allow_credentials) {
            response.headers["Access-Control-Allow-Credentials"] = "true";
        }

        // 3. Handle OPTIONS preflight intercept
        if (request.method == http::HttpMethod::OPTIONS) {
            response.headers["Access-Control-Allow-Methods"] = methods_str;
            response.headers["Access-Control-Allow-Headers"] = headers_str;
            response.headers["Access-Control-Max-Age"] = "86400"; // Cache for 24 hours
            
            response.status(http::HttpStatus::NoContent).send("");
            return false; // Intercepted, stop pipeline
        }

        return true; // Let the actual request proceed to the route handler
    };
}

} // namespace middleware
