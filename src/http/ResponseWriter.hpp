#pragma once
#include "HttpResponse.hpp"
#include <string_view>
#include <memory>
#include "json.hpp"

namespace network { class Proactor; }
namespace concurrency { class ThreadPool; }

namespace http {

class ResponseWriter {
public:
    virtual ~ResponseWriter() = default;

    // Add a default header that will be included in the final response
    virtual void set_header(const std::string& key, const std::string& value) = 0;

    // Get the underlying Proactor to dispatch async operations (like proxying)
    virtual network::Proactor& proactor() = 0;
    
    // Get the thread pool to offload blocking tasks like DNS resolution
    virtual concurrency::ThreadPool& thread_pool() = 0;

    // Send a complete HTTP response (takes ownership of file descriptors if any)
    virtual void send(HttpResponse&& response) = 0;

    // Send only headers (useful for streaming bodies)
    virtual void send_headers(HttpResponse& response) = 0;

    // Stream a chunk of data (for Chunked Transfer Encoding)
    virtual void write_chunk(std::string_view chunk) = 0;

    // End a chunked stream
    virtual void end() = 0;

    // Take over the connection for raw bi-directional byte streaming (e.g. WebSocket Proxying)
    virtual void upgrade_to_raw_stream(std::function<void(std::string_view)> on_data, std::function<void()> on_close) = 0;
};

} // namespace http
