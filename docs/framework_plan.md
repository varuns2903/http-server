# 🚀 C++ Web Framework Development Plan

Transforming the custom HTTP server into an easy-to-use C++ Web Framework requires decoupling the underlying `epoll` engine from the user's application logic and providing a fluent API. The goal is to allow developers to build web services in C++ with the ease of Express.js or Flask.

## Phase 1: API Redesign & Decoupling [COMPLETED]
Currently, the application is wired together in `main.cpp`. We need to hide this complexity behind a unified `Server` or `App` class.

*   **1.1 Unified `App` Interface**
    *   Create a top-level `class App` (or `class Server`) that encapsulates `Listener`, `EventLoop`, `ConnectionManager`, and `ThreadPool`.
    *   Implement fluent routing methods: `app.get()`, `app.post()`, `app.put()`, `app.del()`.
*   **1.2 Callback-Based Request Handling**
    *   Change the `Router` to accept user-defined `std::function` callbacks.
    *   Signature: `using RouteHandler = std::function<void(const HttpRequest&, HttpResponse&)>`.
*   **1.3 Request/Response Ergonomics**
    *   Improve the `HttpResponse` class to support easy JSON serialization (e.g., `.json()`), HTML serving, and status code setters.
    *   Add query parameter parsing to `HttpRequest`.

## Phase 2: Advanced Routing & Middleware [COMPLETED]
A modern web framework needs dynamic routing and the ability to run code before/after requests.

*   **2.1 Dynamic Path Parameters**
    *   Upgrade the `Router` to support path variables like `/users/:id` or `/api/v1/resource/*`.
    *   Populate a `req.params` dictionary that the user can query.
*   **2.2 Middleware Pipeline**
    *   Implement a middleware architecture (`app.use()`).
    *   Allow functions to intercept requests for logging, authentication, or CORS headers before they hit the final route handler.
    *   Implement an `express`-like `next()` function pattern.

## Phase 3: Library Packaging & Build System [PENDING]
To be a true framework, developers need to be able to easily link against it.

*   **3.1 CMake Targets Restructuring**
    *   Ensure the library code is compiled as a `STATIC` (`.a`) or `SHARED` (`.so`/`.dylib`) library.
    *   Create a clear separation between library headers (`include/myframework/`) and source files (`src/`).
*   **3.2 CMake Export & Install Rules**
    *   Write `install(TARGETS ...)` commands in `CMakeLists.txt` so users can run `make install`.
    *   Generate `*Config.cmake` files so users can write `find_package(MyFramework REQUIRED)` in their own projects.
*   **3.3 Header-Only Consideration (Optional)**
    *   Evaluate if a header-only library is viable for easier distribution, though linking a pre-compiled library is faster for user compile times.

## Phase 4: Utilities & Developer Experience (DX) [PARTIALLY COMPLETED]
*   **4.1 Native JSON Support** [COMPLETED]
    *   Integrate a fast JSON library (like `nlohmann/json` or `simdjson`) as a dependency, allowing users to easily parse request bodies and serialize responses.
*   **4.2 Built-in Static File Server** [COMPLETED]
    *   Wrap the existing `sendfile` zero-copy implementation into a middleware: `app.use("/public", static_folder("./public"))`.
*   **4.3 Multi-part Form Parsing** [PENDING]
    *   Add parsing for file uploads (`multipart/form-data`).

## Phase 5: Distribution & Documentation [PENDING]
*   **5.1 Package Managers**
    *   Publish the framework to **vcpkg** and **Conan** so users can install it with a single command.
*   **5.2 Example Projects**
    *   Create an `examples/` directory showing how to build a REST API, a static site, and a WebSocket chat server (if WS support is added).
*   **5.3 Website & API Docs**
    *   Generate API documentation using Doxygen.
    *   Host a clean, readable documentation site.

---

## 🎯 Target Developer Experience (What the user will write)
```cpp
#include <MyFramework/App.hpp>

int main() {
    mf::App app;

    // Middleware
    app.use(mf::logger);

    // Static files
    app.use("/static", mf::static_files("public"));

    // REST API
    app.get("/users/:id", [](const mf::Request& req, mf::Response& res) {
        std::string id = req.params["id"];
        res.json({{"status", "success"}, {"user_id", id}});
    });

    // Start server non-blocking
    app.listen(8080); 
}
```
