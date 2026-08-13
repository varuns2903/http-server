#pragma once
#include <orbit/routing/Router.hpp>

namespace middleware {

/**
 * @ingroup middlewares
 * @brief Middleware for tracking HTTP request metrics.
 */
class Metrics {
public:
    /**
     * @brief Returns a middleware handler that tracks requests.
     * 
     * @return routing::Middleware The metrics tracking middleware handler.
     */
    static routing::Middleware track();
};

} // namespace middleware
