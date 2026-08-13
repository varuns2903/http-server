#pragma once
#include <orbit/routing/Router.hpp>
#include <string>

namespace middleware {

// Returns a middleware that serves static files from the given directory.
// If a file is found, it is served and the pipeline is stopped.
// If a file is not found, the pipeline continues.
routing::Middleware static_files(const std::string& directory);

} // namespace middleware
