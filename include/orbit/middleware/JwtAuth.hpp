#pragma once
#include <orbit/routing/Router.hpp>
#include <string>

namespace middleware {

// JWT Authentication middleware using HMAC-SHA256 (HS256).
// Verifies the "Authorization: Bearer <token>" header against the secret key.
// If valid, the decoded JSON payload is placed into `HttpRequest::user`.
// If invalid or missing, it responds with a 401 Unauthorized and stops the pipeline.
routing::Middleware jwt_auth(const std::string& secret_key);

} // namespace middleware
