#pragma once
#include "../routing/Router.hpp"

namespace middleware {

// Returns a middleware that automatically compresses the HTTP response body using GZIP 
// if the client sent the Accept-Encoding: gzip header.
routing::Middleware compress();

} // namespace middleware
