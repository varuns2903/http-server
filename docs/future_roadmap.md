# Orbit Framework: Future Architecture Roadmap

This document outlines the structured, optimal plan to evolve Orbit from a robust HTTP/1.1 server into an enterprise-grade, high-performance web and reverse-proxy ecosystem.

---

## Phase 6: Production Hardening (The Pending Milestones)
*Goal: Ensure the server cannot be crashed by malicious actors and operates cleanly in a containerized (Docker/Kubernetes) environment.*

### 6.1 Request Limits & DDoS Protection
* **Max Body Size**: Add `max_body_size` to `ServerConfig`. Update `Connection::handle_read()` to track incoming `Content-Length` and chunk sizes. If the buffer exceeds this limit, instantly drop the connection or return `413 Payload Too Large`.
* **Header Limits**: Enforce strict limits on header count (e.g., max 100 headers) and URI length to prevent buffer overflow attacks.
* **Rate Limiting Middleware**: Implement a Token Bucket algorithm middleware to limit requests per IP.

### 6.2 Graceful Shutdown (SIGINT/SIGTERM)
* **Signal Handling**: Use `sigaction` to catch `Ctrl+C` (`SIGINT`) and Docker stops (`SIGTERM`).
* **Connection Draining**: When signaled, the `EventLoop` stops accepting new connections on the `Listener`.
* **Worker Flush**: The `ThreadPool` stops accepting new tasks, but completes all currently enqueued requests.
* **Socket Cleanup**: Active Keep-Alive connections are sent `Connection: close` headers, flushed, and closed.

### 6.3 Automated Benchmarking Suite
* **Tooling Integration**: Add a `benchmarks/` directory with automated scripts using `wrk` or `autocannon`.
* **CI/CD Regression**: Set up GitHub Actions to run the benchmark on every PR and compare Requests-Per-Second (RPS) against the `main` branch to ensure no performance regressions.

---

## Phase 7: Modern Protocols & Security
*Goal: Support encrypted traffic and real-time bidirectional communication.*

### 7.1 TLS / HTTPS Integration
* **OpenSSL / BoringSSL**: Introduce SSL context into the `Listener` and `Connection` classes.
* **I/O Abstraction**: Create a `SocketStream` interface so `Connection::handle_read()` and `handle_write()` can transparently call `SSL_read()`/`SSL_write()` for encrypted sockets, and `recv()`/`send()` for plaintext.
* **ALPN (Application-Layer Protocol Negotiation)**: Essential for later supporting HTTP/2.

### 7.2 WebSockets (RFC 6455)
* **Protocol Upgrade**: When a route receives `Connection: Upgrade` and `Upgrade: websocket`, transition the `Connection` object state out of HTTP mode into a `WebSocketConnection`.
* **Binary Framing**: Implement the WebSocket masking and framing protocol to parse incoming frames.
* **Ergonomics**: Add `app.ws("/chat", [](WebSocket& ws) { ... })` for easy bidirectional message streaming.

---

## Phase 8: Extreme Performance Optimization
*Goal: Push the limits of the Linux Kernel to achieve millions of RPS.*

### 8.1 The `io_uring` Reactor Upgrade
* **Motivation**: `epoll` requires context switching to the kernel to tell us what is ready, followed by `recv`/`send` syscalls. `io_uring` submits I/O operations directly to the kernel via shared memory ring buffers, eliminating syscall overhead.
* **Implementation**: Abstract the `network::Epoll` class into a generic `EventReactor`. Implement an `IoUringReactor` backend using `liburing`.
* **Proactor Pattern**: Shift from a Reactor (epoll says "socket is ready") to a Proactor (io_uring says "I read 4096 bytes directly into your buffer for you").

---

## Phase 9: Scale & Distribution
*Goal: Transform Orbit into a tool capable of replacing NGINX in microservice meshes.*

### 9.1 Reverse Proxying & Load Balancing
* **Upstream Connections**: Allow Orbit to open outbound non-blocking client sockets to upstream servers.
* **Proxy Middleware**: `app.use("/api", middleware::proxy("http://backend-service:5000"))`.
* **Streaming**: Stream data chunk-by-chunk between the client and upstream to avoid holding large payloads in memory.

### 9.2 HTTP/2 & HTTP/3
* **HTTP/2**: Implement HPACK header compression and stream multiplexing (multiple requests over one TCP socket concurrently).
* **HTTP/3 (QUIC)**: Transition away from TCP entirely to use UDP, requiring a custom QUIC stack implementation.

### 9.3 Distributed Operation (Clustering)
* **State Sharing**: Implement a Redis-backed session manager for distributed rate limiting and state across multiple Orbit instances.
