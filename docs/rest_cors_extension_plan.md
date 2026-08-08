# Orbit Framework: REST & CORS Extension Plan

To make Orbit feel as effortless and powerful as Express.js (Node), FastAPI (Python), or Spring Boot (Java), we will implement full REST semantics and built-in cross-origin support.

## Phase 1: Full REST HTTP Methods
Currently, Orbit only supports `GET` and `POST`. We will extend this to support all major REST operations.

### Tasks
1. **Extend Enums**: Add `PUT`, `PATCH`, `DELETE`, and `OPTIONS` to the `HttpMethod` enum in `HttpRequest.hpp`.
2. **Update Parser**: Modify `HttpParser.cpp` to detect and parse the new HTTP method strings from raw socket data.
3. **Router Updates**: Update `Router::make_route_key` to map the new enums to string keys.
4. **App Interface (`App.hpp`)**: Add fluent builder methods for the new verbs:
   - `app.put("/api/resource", ...)`
   - `app.patch("/api/resource", ...)`
   - `app.del("/api/resource", ...)` *(Note: `delete` is a C++ keyword, so `del` is the standard convention).*
   - `app.options("/api/resource", ...)`

## Phase 2: Built-in CORS Support (Middleware)
Modern web browsers block frontend code (React, Vue, etc.) from making requests to a different domain unless the server explicitly allows it via CORS (Cross-Origin Resource Sharing) headers.

We will implement this as a highly configurable **Middleware** so users can enable it with one line of code: `app.use(middleware::cors(...))`.

### Tasks
1. **Create `middleware::CorsOptions` struct**:
   - `std::vector<std::string> allowed_origins` (e.g., `{"*"}` or `{"https://myapp.com"}`)
   - `std::vector<std::string> allowed_methods`
   - `std::vector<std::string> allowed_headers`
   - `bool allow_credentials`
2. **Create `middleware::cors(options)`**:
   - **Preflight Interception**: If the request is an `OPTIONS` request, the middleware will instantly reply with a `204 No Content`, attach all the `Access-Control-*` headers, and halt the pipeline.
   - **Standard Interception**: For normal requests (`GET`, `POST`, etc.), it will attach the `Access-Control-Allow-Origin` header to the response and let the pipeline continue to the user's route.

## Phase 3: Framework Ergonomics (Python/Node DX)
To truly match the ease-of-use of dynamic languages, we need to add a few quality-of-life utilities that developers expect out of the box.

### Tasks
1. **Query String Parsing**:
   - Currently, if a user requests `/search?q=cpp&sort=asc`, the `req.uri` contains the whole string.
   - We will update `HttpParser` to split the URI and the query string.
   - Add `req.query` (`std::unordered_map<std::string, std::string>`) so developers can easily access `req.query.at("q")`.
2. **Global Error Handling**:
   - Wrap the execution of route handlers in a `try/catch` block.
   - If a developer's route throws an unhandled `std::exception`, the framework should automatically catch it, log it, and return a clean `500 Internal Server Error` instead of crashing the whole server.

---
*Ready to execute Phase 1? Click 'Proceed' to begin!*
