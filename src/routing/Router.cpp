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

void Router::use(Middleware m) {
    middlewares_.push_back(std::move(m));
}

void Router::route(http::HttpRequest& request, http::HttpResponse& response) const {
    // 0. Execute Middlewares
    for (const auto& m : middlewares_) {
        if (!m(request, response)) {
            return; // Middleware intercepted and handled the request
        }
    }

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
    
    // 3. If no route matches, return a 404 Not Found
    response.status(http::HttpStatus::NotFound).send("404 Not Found");
}

} // namespace routing
