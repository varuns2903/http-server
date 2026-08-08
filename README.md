# High-Performance C++ Epoll HTTP Server 🚀

A lightning-fast, multi-threaded HTTP/1.1 server built entirely from scratch in C++17. Designed to explore deep systems programming concepts, this server leverages Linux kernel APIs, non-blocking I/O, and zero-copy networking to achieve massive concurrency and blistering speeds.

Benchmarked at **~36,800 Requests Per Second (RPS)** with a mean response time of **0.027ms** on a local machine.

## ✨ Features

- **Asynchronous Event Loop**: Uses Linux `epoll` (`EPOLLIN`, `EPOLLOUT`, `EPOLLONESHOT`) for highly scalable, non-blocking network I/O.
- **Custom Thread Pool**: Offloads request parsing and routing to a dedicated worker thread pool using `std::mutex` and `std::condition_variable`.
- **Zero-Copy File Serving**: Bypasses user-space memory entirely when serving static files by utilizing the Linux `sendfile()` syscall.
- **Zero-Allocation HTTP Parser**: Uses C++17 `std::string_view` to parse HTTP headers and payloads without expensive memory allocations.
- **HTTP Keep-Alive**: Supports persistent connections to reduce TCP handshake overhead.
- **Min-Heap Timer Management**: Automatically closes idle connections using an efficient priority queue-based timeout manager.
- **Security & Path Traversal Prevention**: Uses `std::filesystem::weakly_canonical` to sandbox static file requests and block directory traversal attacks (e.g., `GET /../../../etc/passwd`).
- **Graceful Shutdown**: Intercepts `SIGINT`/`SIGTERM` signals to cleanly drain the thread pool, close file descriptors, and exit safely without memory leaks.
- **Structured Logging**: A thread-safe, ANSI-colored logger tracking filenames, line numbers, and timestamps.
- **CLI Configuration**: Fully configurable via command-line arguments using `getopt_long`.

## 🛠️ Build Instructions

### Prerequisites
- Linux OS (Relies on Linux-specific `epoll` and `sendfile` syscalls)
- CMake (>= 3.10)
- GCC/Clang with C++17 support
- Google Test (Fetched automatically via CMake)

### Compiling
```bash
# Clone the repository
git clone https://github.com/varuns2903/http-server.git
cd http-server

# Build the project
mkdir -p build && cd build
cmake ..
cmake --build .
```

## 🚀 Usage

Start the server using the compiled executable. You can configure the port, worker threads, and static file directory using CLI arguments.

```bash
./build/http_server -p 8080 --threads 8 --log-level INFO --static-dir ./public
```

### CLI Options
| Flag | Long Flag | Description | Default |
|------|-----------|-------------|---------|
| `-p` | `--port`  | Port to bind the server to | `8080` |
| `-t` | `--threads`| Number of worker threads | `4` |
| `-l` | `--log-level`| Logging verbosity (`DEBUG`, `INFO`, `WARN`, `ERROR`) | `INFO` |
| `-s` | `--static-dir`| Directory to serve static assets from | `./public` |
| `-h` | `--help` | Show the help menu | |

## 🧪 Running Tests

The project uses the **Google Test** framework for robust unit testing of the HTTP parser, Router, and Thread Pool logic.

```bash
cd build
ctest --output-on-failure
# Or run the test executable directly
./http_server_tests
```

## 📊 Benchmarks

Stress testing was performed using ApacheBench (`ab`) on a standard Linux development machine. 

**Command:**
```bash
ab -k -c 100 -n 100000 http://127.0.0.1:8080/api/data
```

**Results:**
- **Requests completed:** 100,000
- **Failed requests:** 0
- **Requests per second:** 36,874.84 [#/sec] (mean)
- **Time per request:** 2.712 [ms] (mean)
- **Time per request (across all threads):** 0.027 [ms]

## 🧠 Architecture Overview

1. **Listener**: Binds to a port and listens for incoming TCP connections.
2. **Event Loop (`epoll`)**: Monitors the Listener and all connected client sockets for readability/writability.
3. **Connection Manager**: Maps file descriptors (FDs) to stateful `Connection` objects.
4. **Worker Threads**: When a socket has a complete HTTP request ready, the Event Loop pushes the `Connection` to the Thread Pool. The worker parses the request, consults the `Router`, and buffers the response.
5. **Zero-Copy Transmission**: If a static file is requested, the worker thread opens the file and arms the socket for `EPOLLOUT`. The Event Loop directly streams the file from the kernel buffer to the network card using `sendfile()`, bypassing the C++ application entirely.

## 🛡️ Memory Safety
The entire architecture was aggressively profiled and hardened using **Address Sanitizer (ASAN)** to eliminate use-after-free race conditions, double-frees, and memory leaks in the multithreaded epoll environment.

## 📝 License
This project is open-source and available under the MIT License.
