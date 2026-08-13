#pragma once
#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <orbit/http/HttpRequest.hpp>
#include <orbit/http/ResponseWriter.hpp>
#include <orbit/http/json.hpp>

namespace middleware {

struct OAuth2Config {
    std::string client_id;
    std::string client_secret;
    std::string redirect_uri;
    std::string authorization_endpoint;
    std::string token_endpoint;
    std::string userinfo_endpoint;
    std::vector<std::string> scopes;
};

class OAuth2 {
public:
    OAuth2(const OAuth2Config& config);

    // Handler to redirect the user to the OAuth2 provider's consent page
    std::function<void(http::HttpRequest&, std::shared_ptr<http::ResponseWriter>)> login_handler();

    // Handler to process the redirect callback, exchange the code for an access token, and fetch user info
    std::function<void(http::HttpRequest&, std::shared_ptr<http::ResponseWriter>)> callback_handler(
        std::function<void(const nlohmann::json& user_info, http::HttpRequest&, std::shared_ptr<http::ResponseWriter>)> on_success,
        std::function<void(const std::string& error, http::HttpRequest&, std::shared_ptr<http::ResponseWriter>)> on_error
    );

    // Pre-configured factories for popular providers
    static OAuth2 google(const std::string& client_id, const std::string& client_secret, const std::string& redirect_uri);
    static OAuth2 github(const std::string& client_id, const std::string& client_secret, const std::string& redirect_uri);

private:
    OAuth2Config config_;

    // Synchronous HTTPS fetch helper using libcurl
    std::string fetch_sync(const std::string& url, const std::string& method, const std::string& body, const std::string& auth_header = "") const;
};

} // namespace middleware
