# Reverse Proxy & API Gateway Enhancements

While the foundation of the asynchronous reverse proxy is fully functional and successfully routes requests to upstream servers via the `io_uring`/`epoll` kernel loop, several critical enhancements are required to make it production-ready.

## 1. True Streaming Proxy (Zero-Buffering) [COMPLETED]
**Issue:** The proxy currently buffers the entire upstream HTTP response into a `std::string` in RAM before sending it back to the client. This will cause Out-Of-Memory (OOM) crashes when proxying large files (e.g., video streaming or large file downloads).
**Solution:** Upgrade the proxy to parse the HTTP headers as soon as they arrive from the upstream, forward them to the client using `writer->send_headers()`, and then pipe the body bytes directly to the client via `writer->write_chunk()` (using HTTP Chunked Transfer Encoding if `Content-Length` isn't available).

## 2. Asynchronous DNS Resolution [COMPLETED]
**Issue:** The proxy currently uses `gethostbyname()`, which is a blocking system call. If DNS resolution takes 500ms, the entire kernel event loop will freeze for 500ms, stalling all other concurrent connections.
**Solution:** Dispatch DNS resolution to the background `ThreadPool` or integrate a truly asynchronous DNS resolver (like `c-ares`).

## 3. Upstream Connection Pooling (Keep-Alive) [COMPLETED]
**Issue:** For every proxied request, the gateway opens a brand new TCP connection to the upstream server and tears it down afterwards. This incurs massive latency under high load.
**Solution:** Implement an `UpstreamConnectionPool` that maintains warm, keep-alive TCP connections to backend nodes. The proxy can simply borrow a socket, send a request, and return the socket to the pool.

## 4. Upstream TLS/HTTPS Support [COMPLETED]
**Issue:** The proxy currently only connects to plain-text `http://` upstream servers via raw TCP sockets.
**Solution:** Extend `AsyncClient` to wrap the outbound socket in an OpenSSL `BIO`/`SSL` state machine so we can securely proxy traffic to `https://` backends.

## 5. WebSocket Proxying [COMPLETED]
**Issue:** The proxy currently drops `Upgrade: websocket` headers.
**Solution:** Enhance the proxy state machine to recognize 101 Switching Protocols from the upstream, and seamlessly pipe bi-directional raw WebSocket frames between the client and the upstream server.

## 6. Gateway Headers & Forwarding [COMPLETED]
**Issue:** The backend does not know the true IP of the original client.
**Solution:** The proxy should inject standard Gateway headers (`X-Forwarded-For`, `X-Forwarded-Proto`, `X-Real-IP`, `X-Forwarded-Host`) before transmitting the request to the upstream server.
