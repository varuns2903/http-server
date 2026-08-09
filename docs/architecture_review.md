# Orbit Framework: Comprehensive Architecture Review

Before proceeding to implement the highly complex WebSocket protocol, I have conducted a thorough review of the current codebase architecture. Here is an evaluation of our design patterns, strengths, and areas of technical debt that need addressing.

## 1. System Architecture Strengths

### The Reactor Pattern (`network::Epoll` + `EventLoop`)
* **Status**: Highly Robust
* **Review**: We successfully implemented the classic NGINX/Node.js Reactor pattern. The main thread solely handles `epoll_wait`, reacting to `EPOLLIN` and `EPOLLOUT` events. It never blocks on reading or writing, allowing the server to handle thousands of concurrent idle connections effortlessly.

### Concurrency Model (`concurrency::ThreadPool`)
* **Status**: Optimal
* **Review**: Once a full HTTP request is read from a non-blocking socket, the `EventLoop` dispatches the parsing and routing execution to a lock-free worker pool. This ensures that CPU-intensive route handlers (like JSON parsing or database queries) never stall the main event loop.

### Memory & Resource Safety
* **Status**: Excellent
* **Review**: 
  * Sockets are strictly managed via RAII (`network::Socket`). When a `Connection` is destroyed, the socket FD is automatically closed, preventing file descriptor leaks.
  * The `ConnectionManager` uses `std::unique_ptr<Connection>`, ensuring clean teardowns.
  * We recently resolved the notorious AB/BA deadlock between the `ConnectionManager` and `TimerManager`, making thread interactions mathematically safe.

### Protocol Hardening
* **Status**: Production Ready
* **Review**: 
  * TLS is cleanly integrated using `SSL_ERROR_WANT_READ` and `SSL_ERROR_WANT_WRITE` without breaking the non-blocking paradigm.
  * DDoS protection is active via `max_body_size` checks on the read buffer.
  * `sendfile(2)` zero-copy is successfully implemented for plaintext static files, bypassing user-space entirely.

---

## 2. Technical Debt & Areas for Refactoring

Before we add WebSockets, there are a few architectural bottlenecks in `Connection.cpp` that we should refactor:

### A. The Connection State Machine
**Current State**: `Connection` implicitly assumes every incoming byte belongs to an HTTP request. It checks `is_request_complete()` by looking for `\r\n\r\n` and `Content-Length`.
**The Problem**: If a connection is upgraded to a WebSocket, it is no longer speaking HTTP. It will be speaking binary WebSocket frames.
**The Fix**: We need to introduce an explicit State Machine to `Connection` (e.g., `enum class State { HTTP_READING, HTTP_WRITING, WEBSOCKET }`). If the state is `WEBSOCKET`, `handle_read()` should route bytes to a `WebSocketFrameParser` instead of the `HttpParser`.

### B. HTTP Pipelining Logic
**Current State**: If a client sends multiple HTTP requests in a single TCP packet (pipelining), we erase the first request from the `read_buffer_`, execute it, and then check at the end of `handle_write()` if there's another full request waiting.
**The Problem**: While this works, it can be fragile. If a pipelined request throws a `400 Bad Request`, we currently clear the *entire* buffer, potentially dropping valid pipelined requests that followed it.
**The Fix**: We should decouple the `read_buffer_` from the HTTP request execution so it acts strictly as a byte queue.

### C. Large Request Body Offloading
**Current State**: We buffer the entire `max_body_size` (up to 10MB) in memory in `std::vector<char> read_buffer_`.
**The Problem**: 1,000 concurrent clients uploading 10MB files would consume 10GB of RAM, leading to an Out-Of-Memory (OOM) kill by the OS.
**The Fix (Future)**: Introduce streaming request bodies, where large payloads are piped directly to disk or upstream proxy connections without ever fully buffering in RAM.

---

## Conclusion & Next Steps

The codebase is exceptionally solid for HTTP/1.1 traffic. To support WebSockets safely, **we must implement the State Machine refactor (Point A) first**. This will ensure the `Connection` class cleanly pivots from HTTP logic to WebSocket logic upon receiving a `101 Switching Protocols` response.
