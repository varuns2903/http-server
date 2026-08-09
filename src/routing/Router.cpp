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

bool Router::has_ws_route(const std::string& path) const {
    return ws_routes_.find(path) != ws_routes_.end();
}

WsHandler Router::get_ws_route(const std::string& path) const {
    auto it = ws_routes_.find(path);
    if (it != ws_routes_.end()) {
        return it->second;
    }
    return nullptr;
}

void Router::group(const std::string& prefix, std::function<void(Router&)> callback) {
    Router group_router(prefix, this);
    callback(group_router);
}

void Router::add_route(http::HttpMethod method, const std::string& path, RouteHandler handler) {
    std::string full_path = prefix_ + path;
    
    RouteHandler wrapped = handler;
    if (!local_middlewares_.empty()) {
        wrapped = [mws = local_middlewares_, h = std::move(handler)](http::HttpRequest& req, std::shared_ptr<http::ResponseWriter> w) {
            for (auto& mw : mws) {
                if (!mw(req, w)) return;
            }
            h(req, w);
        };
    }

    if (parent_) {
        parent_->add_route(method, full_path, std::move(wrapped));
        return;
    }

    if (full_path.find(':') != std::string::npos || full_path.find('*') != std::string::npos) {
        DynamicRoute dr;
        dr.method = method;
        dr.path_segments = split_path(full_path);
        dr.handler = std::move(wrapped);
        dynamic_routes_.push_back(std::move(dr));
    } else {
        std::string key = make_route_key(method, full_path);
        routes_[key] = std::move(wrapped);
    }
}

void Router::ws(const std::string& path, WsHandler handler) {
    std::string full_path = prefix_ + path;
    if (parent_) {
        parent_->ws(full_path, std::move(handler));
    } else {
        ws_routes_[full_path] = std::move(handler);
    }
}

void Router::use(Middleware m) {
    if (parent_) {
        local_middlewares_.push_back(std::move(m));
    } else {
        middlewares_.push_back(std::move(m));
    }
}

void Router::route(http::HttpRequest& request, std::shared_ptr<http::ResponseWriter> response_writer) const {
    try {
        // 1. Run global and route-specific middlewares
        for (auto& mw : middlewares_) {
            if (!mw(request, response_writer)) {
                return; // Pipeline stopped by middleware (e.g., auth failed, file served)
            }
        }
        
        std::string route_key = make_route_key(request.method, request.uri);
        
        // 2. Exact match check
        auto it = routes_.find(route_key);
        if (it != routes_.end()) {
            it->second(request, response_writer);
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
                dr.handler(request, response_writer);
                return;
            }
        }
        
        // 4. If no route matches, return a 404 Not Found
        http::HttpResponse res;
        res.status(http::HttpStatus::NotFound).send("404 Not Found");
        response_writer->send(std::move(res));
        
    } catch (const std::exception& e) {
        LOG_ERROR("Unhandled exception in route " << request.uri << ": " << e.what());
        http::HttpResponse res;
        res.status(http::HttpStatus::InternalServerError).send(
            "500 Internal Server Error: " + std::string(e.what())
        );
        response_writer->send(std::move(res));
    } catch (...) {
        LOG_ERROR("Unknown unhandled exception in route " << request.uri);
        http::HttpResponse res;
        res.status(http::HttpStatus::InternalServerError).send("500 Internal Server Error");
        response_writer->send(std::move(res));
    }
}

} // namespace routing
