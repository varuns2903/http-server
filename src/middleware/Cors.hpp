#pragma once
#include "../routing/Router.hpp"
#include <string>
#include <vector>

namespace middleware {

struct CorsOptions {
    std::vector<std::string> allowed_origins = {"*"};
    std::vector<std::string> allowed_methods = {"GET", "POST", "PUT", "PATCH", "DELETE", "OPTIONS"};
    std::vector<std::string> allowed_headers = {"*"};
    bool allow_credentials = false;
};

// Returns a middleware that handles CORS (Cross-Origin Resource Sharing)
routing::Middleware cors(CorsOptions options = CorsOptions{});

} // namespace middleware
