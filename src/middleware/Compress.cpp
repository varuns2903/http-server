#include "Compress.hpp"
#include <zlib.h>
#include <cstring>
#include <iostream>

namespace middleware {

routing::Middleware compress() {
    return [](http::HttpRequest& request, std::shared_ptr<http::ResponseWriter> writer) -> bool {
        auto ae = request.headers.find("Accept-Encoding");
        if (ae != request.headers.end() && ae->second.find("gzip") != std::string_view::npos) {
            
            // Add an interceptor to the ResponseWriter to catch the outgoing response
            writer->add_interceptor([](http::HttpResponse& res) {
                // Don't compress empty bodies or raw file descriptors
                if (res.body.empty() || res.file_fd != -1) return;
                
                // Don't compress very small payloads (overhead > savings)
                if (res.body.size() < 150) return;
                
                // Don't compress already compressed formats
                auto ct_it = res.headers.find("Content-Type");
                if (ct_it != res.headers.end()) {
                    if (ct_it->second.find("image") != std::string::npos || 
                        ct_it->second.find("video") != std::string::npos ||
                        ct_it->second.find("application/zip") != std::string::npos ||
                        ct_it->second.find("application/gzip") != std::string::npos) {
                        return;
                    }
                }
                
                z_stream zs;
                std::memset(&zs, 0, sizeof(zs));
                
                // 15 + 16 enables gzip envelope
                if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
                    return;
                }
                
                zs.next_in = (Bytef*)res.body.data();
                zs.avail_in = static_cast<uInt>(res.body.size());
                
                int ret;
                char outbuffer[32768];
                std::string compressed_body;
                
                do {
                    zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
                    zs.avail_out = sizeof(outbuffer);
                    
                    ret = deflate(&zs, Z_FINISH);
                    
                    if (compressed_body.size() < zs.total_out) {
                        compressed_body.append(outbuffer, zs.total_out - compressed_body.size());
                    }
                } while (ret == Z_OK);
                
                deflateEnd(&zs);
                
                if (ret == Z_STREAM_END) {
                    res.body = std::move(compressed_body);
                    res.headers["Content-Encoding"] = "gzip";
                    res.headers["Content-Length"] = std::to_string(res.body.size());
                }
            });
        }
        return true;
    };
}

} // namespace middleware
