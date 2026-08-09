#include "StaticFiles.hpp"
#include "../utils/Logger.hpp"
#include <filesystem>

namespace middleware {

routing::Middleware static_files(const std::string& directory) {
    return [directory](http::HttpRequest& request, std::shared_ptr<http::ResponseWriter> writer) -> bool {
        if (request.method != http::HttpMethod::GET) {
            return true; // Continue pipeline, only GET requests are served statically
        }
        
        try {
            namespace fs = std::filesystem;
            fs::path base_path = fs::canonical(directory);
            
            // Remove leading '/' to make the path relative
            std::string rel_path = request.uri.empty() ? "" : std::string(request.uri.substr(1));
            if (rel_path.empty()) rel_path = "index.html"; 
            
            fs::path requested_path = fs::weakly_canonical(base_path / rel_path);
            
            // SECURITY CHECK: Prevent Path Traversal
            std::string base_str = base_path.string();
            std::string req_str = requested_path.string();
            
            if (req_str == base_str || req_str.find(base_str + "/") == 0) {
                if (fs::is_regular_file(requested_path)) {
                    std::string ext = requested_path.extension().string();
                    std::string mime = "application/octet-stream";
                    if (ext == ".html") mime = "text/html";
                    else if (ext == ".css") mime = "text/css";
                    else if (ext == ".js") mime = "application/javascript";
                    else if (ext == ".json") mime = "application/json";
                    else if (ext == ".png") mime = "image/png";
                    else if (ext == ".jpg" || ext == ".jpeg") mime = "image/jpeg";
                    else if (ext == ".txt") mime = "text/plain";
                    
                    http::HttpResponse res;
                    res.send_file(req_str, mime);
                    writer->send(std::move(res));
                    return false; // File served, stop pipeline!
                }
            } else {
                LOG_WARN("Path traversal attack blocked! Attempted to access: " << request.uri);
                http::HttpResponse res;
                res.status(http::HttpStatus::Forbidden).send("403 Forbidden");
                writer->send(std::move(res));
                return false; // Handled as error, stop pipeline!
            }
        } catch (const std::exception& e) {
            // File not found or directory does not exist, fall through to next middleware/route
        }
        
        return true; // File not found, continue pipeline
    };
}

} // namespace middleware
