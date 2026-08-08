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
        case http::HttpMethod::PUT: method_str = "PUT"; break;
        case http::HttpMethod::PATCH: method_str = "PATCH"; break;
        case http::HttpMethod::DELETE: method_str = "DELETE"; break;
        case http::HttpMethod::OPTIONS: method_str = "OPTIONS"; break;
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
    try {
        // 1. Run global and route-specific middlewares
        for (auto& mw : middlewares_) {
            if (!mw(request, response)) {
                return; // Pipeline stopped by middleware (e.g., auth failed, file served)
            }
        }
        
        std::string route_key = make_route_key(request.method, request.uri);
        
        // 2. Exact match check
        auto it = routes_.find(route_key);
        if (it != routes_.end()) {
            it->second(request, response);
            return;
        }
        
        // 3. Dynamic match check (e.g. /users/:id)
        auto req_segments = split_path(request.uri);
        for (const auto& dr : dynamic_routes_) {
            if (dr.method != request.method) continue;
            
            std::unordered_map<std::string, std::string> extracted_params;
            bool matches = true;
            
            if (req_segments.size() != dr.path_segments.size()) continue;
            
            for (size_t i = 0; i < dr.path_segments.size(); ++i) {
                if (dr.path_segments[i][0] == ':') {
                    // It's a parameter! Extract it.
                    std::string param_name = dr.path_segments[i].substr(1);
                    extracted_params[param_name] = req_segments[i];
                } else if (dr.path_segments[i] != req_segments[i]) {
                    // Static segment mismatch
                    matches = false;
                    break;
                }
            }
            
            if (matches) {
                request.params = std::move(extracted_params);
                dr.handler(request, response);
                return;
            }
        }
        
        // 4. If no route matches, return a 404 Not Found
        response.status(http::HttpStatus::NotFound).send("404 Not Found");
        
    } catch (const std::exception& e) {
        LOG_ERROR("Unhandled exception in route " << request.uri << ": " << e.what());
        response.status(http::HttpStatus::InternalServerError).send(
            "500 Internal Server Error: " + std::string(e.what())
        );
    } catch (...) {
        LOG_ERROR("Unknown unhandled exception in route " << request.uri);
        response.status(http::HttpStatus::InternalServerError).send("500 Internal Server Error");
    }
}

} // namespace routing
