#pragma once
#include <orbit/routing/Router.hpp>
#include <string>

namespace middleware {

/**
 * @ingroup middlewares
 * @brief Returns a middleware that serves static files from a directory.
 * 
 * If a file is found, it is served and the pipeline is stopped.
 * If a file is not found, the pipeline continues.
 * 
 * @param directory The directory containing static files.
 * @return routing::Middleware The static files middleware handler.
 */
routing::Middleware static_files(const std::string& directory);

} // namespace middleware
