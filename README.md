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
| **Prometheus Metrics** | 🟢 Active | **Database (Redis/PG)** | 🟢 Active |
| **TLS/SSL Encryption** | 🟢 Active | **Static File Server** | 🟢 Active |

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
#include "middleware/Cors.hpp"
#include "middleware/Metrics.hpp"
#include "middleware/RateLimiter.hpp"

int main() {
    server::App app;

    // 🛡️ Apply Global Middlewares
    app.use(middleware::Cors::allow_all());
    app.use(middleware::Metrics::track());
    app.use(middleware::rate_limit(100, std::chrono::seconds(10)));
    
    // 📊 Enable Prometheus /metrics endpoint
    app.enable_metrics(); 

    // 🛣️ Create Route Group
    auto api = app.group("/api/v1");

    // ⚡ Dynamic Routing with Parameter Extraction
    api->get("/users/:id", [](http::HttpRequest& req, std::shared_ptr<http::ResponseWriter> res) {
        res->json({
            {"status", "success"}, 
            {"user_id", req.params["id"]}
        });
    });

    // 🚀 Start the server
    app.listen(3000, []() {
        std::cout << "Server listening on port 3000!" << std::endl;
    });

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
