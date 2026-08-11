#pragma once
#include "../routing/Router.hpp"
#include <vector>

namespace middleware {

struct ProxyOptions {
    std::string target_host;
    int target_port;
    std::string strip_prefix = "";
};

// Returns a middleware that proxies requests to a single target URL (ip/hostname and port).
routing::Middleware proxy(ProxyOptions options);
routing::Middleware proxy(const std::string& target_host, int target_port);

struct TargetNode {
    std::string host;
    int port;
};

struct LoadBalancerOptions {
    std::vector<TargetNode> nodes;
    std::string strip_prefix = "";
};

// Returns a middleware that load-balances requests across multiple upstream nodes
// Currently implements Round-Robin algorithm.
routing::Middleware load_balancer(LoadBalancerOptions options);
routing::Middleware load_balancer(const std::vector<TargetNode>& nodes);

} // namespace middleware
