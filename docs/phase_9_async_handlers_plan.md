# Phase 9.0: Asynchronous Request Handlers & Reverse Proxy Foundation [COMPLETED]

## The Problem
Currently, Orbit Framework routes requests synchronously. `Connection::process_request()` expects the `Router` to populate an `HttpResponse` object immediately:
```cpp
http::HttpResponse response;
router_.route(req, response);
send_data(response.serialize());
```
This design completely blocks the development of **Reverse Proxying**, **Database I/O**, or any asynchronous logic because the handler cannot yield execution and resume later. If a handler tries to fetch data from an upstream server, it would block the worker thread, defeating the purpose of our non-blocking Proactor architecture.

## The Solution: `ResponseWriter` (Express.js Model)
We will introduce a `ResponseWriter` interface that handlers use to explicitly send responses. This allows handlers to respond synchronously *or* asynchronously.

### 1. The Interface
```cpp
namespace http {
class ResponseWriter {
public:
    virtual ~ResponseWriter() = default;

    // Send a complete HTTP response
    virtual void send(const HttpResponse& response) = 0;

    // Stream a chunk of data (for Chunked Transfer Encoding or proxying)
    virtual void write_chunk(std::string_view chunk) = 0;

    // End a chunked stream
    virtual void end() = 0;
};
}
```

### 2. Implementation in `Connection`
Since `Connection` inherits from `std::enable_shared_from_this<Connection>`, it can safely implement `ResponseWriter`.
- `Connection::send(response)` will serialize the response, append it to `write_buffer_`, and call `trigger_write()`. It will also re-arm the read pipeline for Keep-Alive pipelining!
- By passing `std::shared_ptr<ResponseWriter>` to the route handlers, the `Connection` object will be kept alive automatically even if the downstream client disconnects early, preventing dangling references during upstream proxy processing.

### 3. Router Signature Update
The `RouteHandler` and `Middleware` signatures will change:
```cpp
using RouteHandler = std::function<void(const http::HttpRequest&, std::shared_ptr<http::ResponseWriter>)>;
using Middleware = std::function<bool(http::HttpRequest&, std::shared_ptr<http::ResponseWriter>)>;
```

## Phase 9.1: The Reverse Proxy Middleware
Once handlers can be asynchronous, implementing the reverse proxy becomes straightforward:
### Step 2: Reverse Proxying & Distributed Systems [COMPLETED]
- Build a generic `middleware::proxy` that takes a target host and port.
- Implement an `AsyncClient` pattern inside the proxy.
- Forward requests using asynchronous sockets, wait for the response via `Epoll`/`io_uring`.
- Forward response back to the client using the new `ResponseWriter` interface.

## Next Steps
1. Create `src/http/ResponseWriter.hpp`.
2. Update `Connection.hpp` and `Connection.cpp` to implement it.
3. Refactor `Router` and all existing handlers (StaticFiles, Cors, basic_server.cpp) to use the new signature.
4. Verify all tests pass with the asynchronous paradigm.
