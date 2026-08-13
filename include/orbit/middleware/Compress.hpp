#pragma once
#include <orbit/routing/Router.hpp>

namespace middleware {

/**
 * @defgroup middlewares Middlewares
 * @brief Built-in middleware components for Orbit HTTP server.
 */

/**
 * @ingroup middlewares
 * @brief Returns a middleware that automatically compresses the HTTP response body using GZIP.
 *
 * Compresses the response if the client sent the Accept-Encoding: gzip header.
 *
 * @return routing::Middleware The compression middleware handler.
 */
routing::Middleware compress();

} // namespace middleware
