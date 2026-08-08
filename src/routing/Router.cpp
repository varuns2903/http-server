#include "Router.hpp"
#include "../utils/Logger.hpp"
#include <filesystem>

namespace routing {

std::string Router::make_route_key(http::HttpMethod method, std::string_view path) const {
    std::string method_str;
    switch (method) {
        case http::HttpMethod::GET: method_str = "GET"; break;
        case http::HttpMethod::POST: method_str = "POST"; break;
        case http::HttpMethod::HEAD: method_str = "HEAD"; break;
        default: method_str = "UNKNOWN"; break;
    }
    return method_str + " " + std::string(path);
}

void Router::add_route(http::HttpMethod method, const std::string& path, RouteHandler handler) {
    std::string key = make_route_key(method, path);
    routes_[key] = std::move(handler);
}

void Router::set_static_dir(const std::string& dir) {
    static_dir_ = dir;
}

void Router::route(const http::HttpRequest& request, http::HttpResponse& response) const {
    std::string key = make_route_key(request.method, request.uri);
    
    auto it = routes_.find(key);
    if (it != routes_.end()) {
        // We found a matching route, execute the handler!
        it->second(request, response);
        return;
    }
    
    // Fallback: Check if it's a request for a static file
    if (!static_dir_.empty() && request.method == http::HttpMethod::GET) {
        try {
            namespace fs = std::filesystem;
            fs::path base_path = fs::canonical(static_dir_);
            
            // Remove leading '/' from request.uri to make it relative
            std::string rel_path = request.uri.empty() ? "" : std::string(request.uri.substr(1));
            if (rel_path.empty()) rel_path = "index.html"; // Default file
            
            fs::path requested_path = fs::weakly_canonical(base_path / rel_path);
            
            // SECURITY CHECK: Prevent Path Traversal
            // Ensure the resolved requested path strictly starts with the base_path
            std::string base_str = base_path.string();
            std::string req_str = requested_path.string();
            
            if (req_str.find(base_str) == 0) {
                if (fs::is_regular_file(requested_path)) {
                    std::string ext = requested_path.extension().string();
                    std::string mime = "application/octet-stream";
                    if (ext == ".html") mime = "text/html";
                    else if (ext == ".css") mime = "text/css";
                    else if (ext == ".js") mime = "application/javascript";
                    else if (ext == ".json") mime = "application/json";
                    else if (ext == ".png") mime = "image/png";
                    else if (ext == ".jpg" || ext == ".jpeg") mime = "image/jpeg";
                    else if (ext == ".txt") mime = "text/plain";
                    
                    response.send_file(req_str, mime);
                    return;
                }
            } else {
                LOG_WARN("Path traversal attack blocked! Attempted to access: " << request.uri);
                response.status(http::HttpStatus::Forbidden).send("403 Forbidden");
                return;
            }
        } catch (const std::exception& e) {
            // File not found or filesystem error, fall through to 404
        }
    }
    
    // If no route matches, return a 404 Not Found
    response.status(http::HttpStatus::NotFound).send("404 Not Found");
}

} // namespace routing
