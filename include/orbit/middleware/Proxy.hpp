#pragma once
#include <orbit/routing/Router.hpp>
#include <vector>

namespace middleware {

/**
 * @ingroup middlewares
 * @brief Options for Proxy middleware.
 */
struct ProxyOptions {
    std::string target_host;
    int target_port;
    std::string strip_prefix = "";
};

/**
 * @ingroup middlewares
 * @brief Returns a middleware that proxies requests to a single target URL (ip/hostname and port).
 * 
 * @param options The proxy configuration options.
 * @return routing::Middleware The proxy middleware handler.
 */
routing::Middleware proxy(ProxyOptions options);

/**
 * @ingroup middlewares
 * @brief Returns a middleware that proxies requests to a single target URL.
 * 
 * @param target_host The target host.
 * @param target_port The target port.
 * @return routing::Middleware The proxy middleware handler.
 */
routing::Middleware proxy(const std::string& target_host, int target_port);

/**
 * @ingroup middlewares
 * @brief A target node for the load balancer.
 */
struct TargetNode {
    std::string host;
    int port;
};

/**
 * @ingroup middlewares
 * @brief Options for LoadBalancer middleware.
 */
struct LoadBalancerOptions {
    std::vector<TargetNode> nodes;
    std::string strip_prefix = "";
};

/**
 * @ingroup middlewares
 * @brief Returns a middleware that load-balances requests across multiple upstream nodes.
 * 
 * Currently implements a Round-Robin algorithm.
 * 
 * @param options The load balancer options.
 * @return routing::Middleware The load balancer middleware handler.
 */
routing::Middleware load_balancer(LoadBalancerOptions options);

/**
 * @ingroup middlewares
 * @brief Returns a middleware that load-balances requests across multiple upstream nodes.
 * 
 * @param nodes A vector of target nodes.
 * @return routing::Middleware The load balancer middleware handler.
 */
routing::Middleware load_balancer(const std::vector<TargetNode>& nodes);

} // namespace middleware
