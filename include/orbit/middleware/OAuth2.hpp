#pragma once
#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <orbit/http/HttpRequest.hpp>
#include <orbit/http/ResponseWriter.hpp>
#include <orbit/http/json.hpp>

namespace middleware {

/**
 * @ingroup middlewares
 * @brief Configuration for OAuth2 middleware.
 */
struct OAuth2Config {
    std::string client_id;
    std::string client_secret;
    std::string redirect_uri;
    std::string authorization_endpoint;
    std::string token_endpoint;
    std::string userinfo_endpoint;
    std::vector<std::string> scopes;
};

/**
 * @ingroup middlewares
 * @brief Middleware for handling OAuth2 authentication.
 */
class OAuth2 {
public:
    /**
     * @brief Constructs an OAuth2 middleware with the given config.
     * 
     * @param config The OAuth2 configuration.
     */
    OAuth2(const OAuth2Config& config);

    /**
     * @brief Handler to redirect the user to the OAuth2 provider's consent page.
     * 
     * @return std::function A middleware handler for login redirection.
     */
    std::function<void(http::HttpRequest&, std::shared_ptr<http::ResponseWriter>)> login_handler();

    /**
     * @brief Handler to process the redirect callback, exchange the code for an access token, and fetch user info.
     * 
     * @param on_success Callback executed upon successful authentication.
     * @param on_error Callback executed if an error occurs during authentication.
     * @return std::function A middleware handler for the callback route.
     */
    std::function<void(http::HttpRequest&, std::shared_ptr<http::ResponseWriter>)> callback_handler(
        std::function<void(const nlohmann::json& user_info, http::HttpRequest&, std::shared_ptr<http::ResponseWriter>)> on_success,
        std::function<void(const std::string& error, http::HttpRequest&, std::shared_ptr<http::ResponseWriter>)> on_error
    );

    /**
     * @brief Pre-configured factory for Google OAuth2.
     * 
     * @param client_id The Google client ID.
     * @param client_secret The Google client secret.
     * @param redirect_uri The redirect URI.
     * @return OAuth2 A configured OAuth2 instance.
     */
    static OAuth2 google(const std::string& client_id, const std::string& client_secret, const std::string& redirect_uri);

    /**
     * @brief Pre-configured factory for GitHub OAuth2.
     * 
     * @param client_id The GitHub client ID.
     * @param client_secret The GitHub client secret.
     * @param redirect_uri The redirect URI.
     * @return OAuth2 A configured OAuth2 instance.
     */
    static OAuth2 github(const std::string& client_id, const std::string& client_secret, const std::string& redirect_uri);

private:
    OAuth2Config config_;
    std::string fetch_sync(const std::string& url, const std::string& method, const std::string& body, const std::string& auth_header = "") const;
};

} // namespace middleware
