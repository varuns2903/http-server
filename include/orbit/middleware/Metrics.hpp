#pragma once
#include <orbit/routing/Router.hpp>

namespace middleware {

class Metrics {
public:
    static routing::Middleware track();
};

} // namespace middleware
