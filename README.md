<div align="center">
  
  # 🚀 Orbit Server Control Panel
  
  <p><b>A blazing fast, asynchronous, and middleware-driven C++20 HTTP/3 web framework</b></p>
  
  <p>
    <img src="https://img.shields.io/badge/C%2B%2B-20-blue.svg?style=for-the-badge&logo=c%2B%2B" />
    <img src="https://img.shields.io/badge/Engine-io__uring%20%7C%20epoll-orange.svg?style=for-the-badge&logo=linux" />
    <img src="https://img.shields.io/badge/Protocol-HTTP%2F3%20%7C%20QUIC-purple.svg?style=for-the-badge" />
    <img src="https://img.shields.io/badge/Status-Active-brightgreen.svg?style=for-the-badge" />
  </p>
</div>

<br/>

<table>
  <tr>
    <td width="270" valign="top">
      <h3>🧭 Navigation Menu</h3>
      <ul>
        <li><a href="#-dashboard-capability-matrix">📊 Dashboard</a></li>
        <li><a href="#-terminal-quick-start">💻 Terminal</a></li>
        <li><a href="#-editor-maincpp">📝 Editor</a></li>
      </ul>
      <hr/>
      <h3>📖 Documentation</h3>
      <ul>
        <li><a href="docs/getting_started.md">Getting Started</a></li>
        <li><a href="docs/routing.md">Routing & Streaming</a></li>
        <li><a href="docs/middleware.md">Middleware & Validation</a></li>
        <li><a href="docs/database.md">DB & C++20 Coroutines</a></li>
        <li><a href="docs/proxy.md">API Gateway & LB</a></li>
        <li><a href="docs/websockets.md">WebSockets</a></li>
        <li><a href="docs/http3.md">HTTP/3 & QUIC</a></li>
      </ul>
      <hr/>
      <h3>⚙️ System Requirements</h3>
      <ul>
        <li><code>C++20 Compiler (GCC/Clang)</code></li>
        <li><code>Linux 5.6+ (io_uring)</code></li>
        <li><code>CMake 3.15+</code></li>
        <li><code>OpenSSL</code></li>
        <li><code>Hiredis</code></li>
        <li><code>liburing</code></li>
      </ul>
      <hr/>
      <h3>🧪 Diagnostics</h3>
      <ul>
        <li><code>make e2e-test</code></li>
        <li><code>make benchmark</code></li>
      </ul>
    </td>
    <td valign="top">

### 📊 Dashboard: Capability Matrix

| Core Network | Status | Framework Modules | Status |
| :--- | :---: | :--- | :---: |
| **HTTP/1.1 & HTTP/2** | 🟢 Active | **Routing (Express-style)** | 🟢 Active |
| **HTTP/3 & QUIC** | 🟢 Active | **WebSockets (RFC-compliant)**| 🟢 Active |
| **Zero-Downtime Reload**| 🟢 Active | **Middleware Stack** | 🟢 Active |
| **Prometheus Metrics** | 🟢 Active | **Database (Redis/PG/Coro)** | 🟢 Active |
| **TLS/SSL Encryption** | 🟢 Active | **Static File Server** | 🟢 Active |
| **Connection Pooling** | 🟢 Active | **JSON Schema Validation** | 🟢 Active |

### 💻 Terminal: Quick Start

```bash
root@orbit-server:~# mkdir build && cd build
root@orbit-server:~/build# cmake -DCMAKE_BUILD_TYPE=Release ..
root@orbit-server:~/build# make -j$(nproc)
root@orbit-server:~/build# ./basic_server --port 3000 --engine io_uring

[INFO] TLS Context initialized successfully
[INFO] HTTP/3 QUIC enabled on UDP port 3000
[INFO] Event loop started (io_uring). Listening on 3000...
```

    </td>
  </tr>
</table>

<table>
  <tr>
    <td>
      <h3>📝 Editor: <code>main.cpp</code></h3>
      
```cpp
#include "server/App.hpp"
#include "middleware/Validation.hpp"
#include "database/PostgresCoro.hpp"
#include "concurrency/Task.hpp"

using namespace server;
using namespace http;
using namespace middleware;

int main() {
    App app;

    // 🛡️ Global Error Handler
    app.on_error([](const std::exception& e, HttpRequest& req, std::shared_ptr<ResponseWriter> writer) {
        writer->send(HttpResponse().status(HttpStatus::InternalServerError).send("Crash prevented!"));
    });

    // ⚡ Automated JSON Validation
    std::vector<SchemaField> user_schema = {
        {"username", JsonType::STRING, true},
        {"age", JsonType::NUMBER, true}
    };

    app.post("/users", {validate_json(user_schema)}, [](HttpRequest& req, std::shared_ptr<ResponseWriter> res) {
        std::string username = req.json()["username"];
        res->json({{"status", "created"}, {"username", username}});
    });

    // 🚀 C++20 Coroutines (Non-Blocking async/await)
    app.get("/db", [](HttpRequest& req, std::shared_ptr<ResponseWriter> res) -> concurrency::Task {
        auto pg = std::make_shared<database::PostgresClient>(&res->proactor(), "dbname=postgres");
        if (co_await database::connect_async(pg)) {
            PGresult* db_res = co_await database::query_async(pg, "SELECT current_timestamp;");
            res->send(HttpResponse().status(HttpStatus::OK).send("DB Time: " + std::string(PQgetvalue(db_res, 0, 0))));
        }
    });

    app.listen(3000, []() { std::cout << "Listening on port 3000!" << std::endl; });
    return 0;
}
```

    </td>
  </tr>
</table>

<details>
<summary><b>⚙️ Advanced Configurations & Architecture Details</b></summary>
<br/>

Orbit is designed around a highly scalable, multi-threaded **Proactor pattern**:

- **Event Loop**: Listens for socket readiness natively using `io_uring` (or `epoll` fallback) for maximum kernel-level asynchronous throughput.
- **Connection Manager**: Handles TCP/QUIC socket lifecycles, HTTP Keep-Alive, and connection draining during hot-reloads.
- **Thread Pool**: Offloads HTTP request parsing, middleware execution, and route handling to worker threads, preventing event loop blocking.
- **Router**: Resolves API endpoints rapidly with `O(1)` or `O(log N)` complexity.
- **Middleware Pluggability**: Features built-in modules like global rate-limiting, Redis-backed distributed sessions, zero-copy static file serving, and JWT authentication.

</details>

<div align="center">
  <br/>
  <i>Distributed under the MIT License.</i>
</div>
