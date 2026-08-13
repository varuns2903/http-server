#pragma once
#include <orbit/routing/Router.hpp>
#include <string>

namespace middleware {

/**
 * @ingroup middlewares
 * @brief JWT Authentication middleware using HMAC-SHA256 (HS256).
 * 
 * Verifies the "Authorization: Bearer <token>" header against the secret key.
 * If valid, the decoded JSON payload is placed into `HttpRequest::user`.
 * If invalid or missing, it responds with a 401 Unauthorized and stops the pipeline.
 * 
 * @code
 * app.use(middleware::jwt_auth("my_super_secret_key"));
 * @endcode
 * 
 * @param secret_key The secret key used for HMAC-SHA256 verification.
 * @return routing::Middleware The JWT authentication middleware handler.
 */
routing::Middleware jwt_auth(const std::string& secret_key);

} // namespace middleware
