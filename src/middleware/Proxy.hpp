#pragma once
#include "../routing/Router.hpp"
#include <string>

namespace middleware {

// Returns a middleware that proxies requests to the target URL (ip/hostname and port).
// Because the proxy happens asynchronously over the Proactor, it does not block the thread pool.
routing::Middleware proxy(const std::string& target_host, int target_port);

} // namespace middleware
