#pragma once
#include <string>
#include <string_view>
#include <memory>
#include <orbit/http/HttpRequest.hpp>
#include <orbit/http/ResponseWriter.hpp>
#include <functional>

namespace middleware {

class Csrf {
public:
    Csrf(const std::string& cookie_name = "csrf_token", const std::string& header_name = "X-CSRF-Token");
    bool operator()(http::HttpRequest& req, std::shared_ptr<http::ResponseWriter> writer);

    static std::string generate_random_token();

private:
    std::string cookie_name_;
    std::string header_name_;
};

inline auto csrf_protection(const std::string& cookie_name = "csrf_token", const std::string& header_name = "X-CSRF-Token") {
    return [c = std::make_shared<Csrf>(cookie_name, header_name)](http::HttpRequest& req, std::shared_ptr<http::ResponseWriter> writer) -> bool {
        return (*c)(req, writer);
    };
}

} // namespace middleware
