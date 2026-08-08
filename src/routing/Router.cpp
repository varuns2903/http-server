#include "Router.hpp"
#include "../utils/Logger.hpp"
#include <filesystem>
#include <sstream>

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

std::vector<std::string> Router::split_path(std::string_view path) const {
    std::vector<std::string> segments;
    std::string path_str(path);
    std::istringstream stream(path_str);
    std::string segment;
    while (std::getline(stream, segment, '/')) {
        if (!segment.empty()) {
            segments.push_back(segment);
        }
    }
    return segments;
}

void Router::add_route(http::HttpMethod method, const std::string& path, RouteHandler handler) {
    if (path.find(':') != std::string::npos || path.find('*') != std::string::npos) {
        DynamicRoute dr;
        dr.method = method;
        dr.path_segments = split_path(path);
        dr.handler = std::move(handler);
        dynamic_routes_.push_back(std::move(dr));
    } else {
        std::string key = make_route_key(method, path);
        routes_[key] = std::move(handler);
    }
}

void Router::set_static_dir(const std::string& dir) {
    static_dir_ = dir;
}

void Router::route(http::HttpRequest& request, http::HttpResponse& response) const {
    std::string key = make_route_key(request.method, request.uri);
    
    // 1. Check exact match
    auto it = routes_.find(key);
    if (it != routes_.end()) {
        it->second(request, response);
        return;
    }
    
    // 2. Check dynamic routes
    auto req_segments = split_path(request.uri);
    for (const auto& dr : dynamic_routes_) {
        if (dr.method != request.method) continue;
        
        bool matches = true;
        std::unordered_map<std::string, std::string> extracted_params;
        
        if (req_segments.size() != dr.path_segments.size() && dr.path_segments.empty()) {
             // Handle root vs non-root empty cases
             if (!req_segments.empty()) matches = false;
        } else if (req_segments.size() != dr.path_segments.size()) {
            // Wait, we need to check if there is a wildcard that allows different sizes.
            // For now, strict size match except for wildcards at the end.
            if (dr.path_segments.empty() || dr.path_segments.back() != "*") {
                matches = false;
            }
        }

        if (matches) {
            for (size_t i = 0; i < dr.path_segments.size(); ++i) {
                const auto& seg_pattern = dr.path_segments[i];
                if (seg_pattern == "*") {
                    break; // Wildcard matches everything after
                }
                
                if (i >= req_segments.size()) {
                    matches = false;
                    break;
                }
                
                const auto& req_seg = req_segments[i];
                if (seg_pattern[0] == ':') {
                    // Extract variable
                    std::string param_name = seg_pattern.substr(1);
                    extracted_params[param_name] = req_seg;
                } else if (seg_pattern != req_seg) {
                    matches = false;
                    break;
                }
            }
        }
        
        if (matches) {
            request.params = std::move(extracted_params);
            dr.handler(request, response);
            return;
        }
    }
    
    // 3. Fallback: Check if it's a request for a static file
    if (!static_dir_.empty() && request.method == http::HttpMethod::GET) {
        try {
            namespace fs = std::filesystem;
            fs::path base_path = fs::canonical(static_dir_);
            
            // Remove leading '/' from request.uri to make it relative
            std::string rel_path = request.uri.empty() ? "" : std::string(request.uri.substr(1));
            if (rel_path.empty()) rel_path = "index.html"; // Default file
            
            fs::path requested_path = fs::weakly_canonical(base_path / rel_path);
            
            // SECURITY CHECK: Prevent Path Traversal
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
    
    // 4. If no route matches, return a 404 Not Found
    response.status(http::HttpStatus::NotFound).send("404 Not Found");
}

} // namespace routing
