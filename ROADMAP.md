# Orbit HTTP Framework Roadmap

This document outlines the strategic future of the Orbit HTTP Framework. The roadmap is divided into five major phases, prioritizing Security, Middleware, and Developer Ergonomics before advancing to new protocol specifications and cross-platform native engines.

---

## Phase 1: Security & Middleware Upgrades (Category 3)
*Goal: Provide robust security primitives and bandwidth optimization out-of-the-box.*

- [x] **Static File ETag Caching:** Implement HTTP caching in `StaticFiles.cpp` by hashing file contents (or using `Last-Modified` timestamps) to return `304 Not Modified` headers, eliminating redundant network transfers.
- [x] **Auto-Compression Middleware (GZIP/Brotli):** Integrate `zlib` to dynamically compress HTTP responses for clients specifying `Accept-Encoding: gzip`, vastly reducing bandwidth footprint.
- [x] **Built-in JWT Authentication:** Develop a middleware utilizing our existing OpenSSL dependency to cryptographically verify HMAC-SHA256 JSON Web Tokens (JWTs) and attach the decoded user identity directly to the `HttpRequest` object.
- [ ] **ACME Auto-TLS (Let's Encrypt):** (Deferred) Implement an automated state machine to provision and renew free SSL/TLS certificates on the fly.

## Phase 2: Web Developer Ergonomics (Category 4)
*Goal: Transition the server into a "batteries-included" web framework.*

- [x] **SSR Template Engine (Inja):** Integrate a Jinja2-style template engine (`inja`) for robust server-side HTML rendering.
- [x] **Async PostgreSQL Client:** Develop a non-blocking SQL query wrapper integrated with the reactor to allow high-throughput database interactions.

## Phase 3: Advanced Protocol Support (Category 1)
*Goal: Stay at the bleeding edge of internet transmission standards.*

- [x] **HTTP/2 Multiplexing:** Upgrade from HTTP/1.1 to HTTP/2 to allow browsers to fetch dozens of assets simultaneously over a single multiplexed TCP connection using binary framing.
- [x] **Server-Sent Events (SSE):** Provide a lightweight, one-way streaming API alternative to WebSockets, perfect for real-time notifications, stock tickers, or LLM text generation streams.
- [ ] **HTTP/3 (QUIC):** Implement UDP-based QUIC transport to virtually eliminate connection latency and packet-loss head-of-line blocking.

## Phase 4: True Cross-Platform Portability (Category 2)
*Goal: Ensure the framework runs natively and asynchronously across all major operating systems.*

- [ ] **macOS / FreeBSD (`kqueue`):** Develop a `KqueueProactor` implementation so the framework can run natively on Apple hardware without Docker or Linux virtual machines.
- [ ] **Windows (`IOCP`):** Develop an Input/Output Completion Ports (`IOCP`) Proactor to bring native, zero-overhead asynchronous performance to Windows environments.

## Phase 5: Observability & DevOps (Category 5)
*Goal: Equip system administrators with enterprise telemetry.*

- [ ] **Prometheus Metrics Endpoint:** Track requests-per-second, memory allocation, and latency distributions, exposing a `/metrics` route for Prometheus/Grafana scraping.
- [ ] **Hot Reloading:** Allow the server to gracefully reload binary updates or configurations without dropping active WebSocket connections or HTTP streams.
