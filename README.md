# 🚀 Orbit HTTP Server

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-20-blue.svg?style=flat-square" alt="C++20">
  <img src="https://img.shields.io/badge/License-MIT-green.svg?style=flat-square" alt="License">
  <img src="https://img.shields.io/badge/Engine-io__uring%20%7C%20epoll-orange.svg?style=flat-square" alt="Engine">
  <img src="https://img.shields.io/badge/Status-Active-brightgreen.svg?style=flat-square" alt="Status">
</p>

> A blazing fast, asynchronous, and middleware-driven C++20 HTTP/WebSocket web framework powered by `io_uring` and `epoll`.

---

## ✨ Features

- ⚡ **Asynchronous Core**: Pluggable event loop engine supporting both modern `io_uring` and legacy `epoll` for maximum throughput and kernel-level asynchronous I/O.
- 🛣️ **Express-style Routing**: Dynamic routing, URL parameter extraction, and route groupings.
- 🔌 **WebSocket Support**: Full RFC-compliant WebSocket integration seamlessly built-in.
- 🔒 **TLS/SSL Encryption**: Built-in OpenSSL-based HTTPS proxying and secure traffic handling.
- 🛡️ **Robust Middlewares**: Composable middleware stack including:
  - Global Rate Limiting
  - Redis-backed Distributed Session Management
  - CORS Headers Support
  - Static File Serving (with zero-copy `sendfile`)
- 🧪 **E2E Testing & Benchmarking**: Dedicated testing suite and automated performance benchmarks using Apache Benchmark.
- 🧵 **Multi-threaded Worker Pool**: Efficiently utilizes CPU cores to process requests concurrently without blocking the event loop.

## 🛠️ Architecture

Orbit is designed around a scalable Proactor pattern:
- **Event Loop**: Listens for socket readiness using `io_uring` (or `epoll`).
- **Connection Manager**: Handles socket lifecycles and HTTP Keep-Alive.
- **Thread Pool**: Offloads request parsing, middleware execution, and route handling to worker threads.
- **Router**: Resolves endpoints with `O(1)` or `O(log N)` complexity.

## 🚀 Quick Start

### Prerequisites
- GCC 11+ or Clang 13+ (C++20 support required)
- CMake 3.15+
- Linux Kernel 5.6+ (for `io_uring` support)
- OpenSSL
- Hiredis
- `liburing`

### Build

```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

### Run the Basic Server
```bash
./build/basic_server --port 3000 --engine io_uring
```

## 📖 Usage Example

```cpp
#include "server/App.hpp"
#include "routing/Router.hpp"

int main() {
    server::App app;

    // Apply Global Middleware
    app.use(middleware::cors());
    app.use(middleware::rate_limit(100, std::chrono::seconds(10)));

    // Create Route Group
    auto api = app.group("/api/v1");

    // Dynamic Route
    api->get("/users/:id", [](http::HttpRequest& req, std::shared_ptr<http::ResponseWriter> res) {
        std::string id = req.params["id"];
        res->send(http::HttpResponse{http::HttpStatus::OK, "User ID: " + id});
    });

    app.listen(3000, []() {
        std::cout << "Server listening on port 3000!" << std::endl;
    });

    return 0;
}
```

## 🧪 Testing and Benchmarking

Run the end-to-end integration test suite:
```bash
make e2e-test
```

Run Apache Benchmark performance tests:
```bash
make benchmark
```

## 📜 License

Distributed under the MIT License.
