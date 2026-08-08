# Orbit: High-Performance C++ Web Framework 🚀

**Orbit** is an ultra-fast, lightweight, and modern C++ web framework built from the ground up for Linux. Powered by `epoll` and `sendfile(2)`, it serves raw performance without compromising on developer experience.

## ✨ Features

- **Blazing Fast Concurrency**: Built on a non-blocking Linux `epoll` Event Loop combined with a thread pool.
- **Express-like Routing**: Register routes effortlessly using modern C++ lambdas.
- **Dynamic Path Parameters**: Seamlessly parse dynamic variables (`/users/:id`).
- **Middleware Pipeline**: Inject powerful request interceptors (e.g., authentication, logging) globally or per-route.
- **Native JSON Support**: Powered by `nlohmann/json`, reading and writing JSON bodies is natively supported.
- **Zero-Copy File Serving**: `sendfile(2)` static file serving out of the box via middleware.
- **Security First**: Built-in path traversal attack prevention.
- **Modern C++20**: Zero dependencies (except for GoogleTest for testing) and designed beautifully.

## 📦 Installation

Orbit is a standard CMake project. You can build and install it globally on your system.

```bash
git clone https://github.com/varuns2903/http-server.git
cd http-server
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

Once installed, you can include Orbit in your own CMake projects:

```cmake
find_package(HttpServer REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app HttpServer::core)
```

## 🚀 Quick Start

Here is how simple it is to get a web server running with Orbit:

```cpp
#include <http-server/server/App.hpp>
#include <http-server/config/Config.hpp>
#include <http-server/middleware/StaticFiles.hpp>
#include <iostream>

int main(int argc, char* argv[]) {
    // 1. Initialize configuration
    auto config = config::ServerConfig::parse(argc, argv);
    server::App app(config);

    // 2. Global Middleware (e.g. Logging)
    app.use([](http::HttpRequest& req, http::HttpResponse& res) {
        std::cout << "[Middleware] " << req.uri << std::endl;
        return true; // Continue pipeline
    });

    // 3. Static File Server
    app.use(middleware::static_files("./public"));

    // 4. API Routes
    app.get("/", [](const http::HttpRequest& req, http::HttpResponse& res) {
        res.html("<h1>Welcome to Orbit!</h1>");
    })
    .get("/users/:id", [](const http::HttpRequest& req, http::HttpResponse& res) {
        std::string user_id = req.params.at("id");
        res.json({
            {"user_id", user_id},
            {"status", "active"}
        });
    })
    .post("/echo", [](const http::HttpRequest& req, http::HttpResponse& res) {
        auto payload = req.json(); // Parses body into nlohmann::json
        payload["received"] = true;
        res.json(payload);
    });

    // 5. Blast off!
    app.listen();
    return 0;
}
```

## 🛠️ Architecture

Orbit strictly decouples its networking architecture from its routing layer:
- `server::App`: The developer-facing frontend wrapper.
- `routing::Router`: Core URL parsing, parameter extraction, and middleware execution.
- `server::EventLoop`: The heart of the network. A non-blocking `epoll` reactor handling thousands of connections.
- `concurrency::ThreadPool`: Worker threads that dequeue HTTP parsing and route execution to keep the EventLoop blazing fast.
- `server::TimerManager`: Background thread ensuring Keep-Alive connections are safely swept and closed when idle.

## 🧪 Running Tests

Orbit comes with a comprehensive suite of tests powered by `GoogleTest`.
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
ctest --output-on-failure
```

---
*Built with ❤️ in C++20 for high-performance systems.*
