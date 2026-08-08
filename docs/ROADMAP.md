# High-Performance HTTP/1.1 Server Development Roadmap

## Project Overview

Phase 0: Project setup and development tooling
↓
Phase 1: Basic blocking TCP server
↓
Phase 2: HTTP/1.1 request parsing
↓
Phase 3: HTTP responses and routing
↓
Phase 4: Non-blocking sockets
↓
Phase 5: Linux epoll event loop
↓
Phase 6: Connection management and persistent connections
↓
Phase 7: Thread pool and concurrent request processing
↓
Phase 8: Static file serving and filesystem security
↓
Phase 9: Request limits, timeouts, error handling and robustness
↓
Phase 10: Logging and configuration
↓
Phase 11: Graceful shutdown and production engineering
↓
Phase 12: Unit, integration and concurrency testing
↓
Phase 13: Benchmarking and profiling
↓
Phase 14: Performance optimization
↓
Phase 15: Final architecture cleanup, documentation and portfolio preparation
↓
**Final Production-Quality Server**

---

## Phase 0: Project setup and development tooling
1. **Primary objective**: Establish a robust C++20 build environment with static analysis, sanitizers, and a testing framework.
2. **Why this sprint exists**: C++ demands strict safety nets. Catching undefined behavior (UB), memory leaks, and memory corruption early saves days of debugging.
3. **Concepts I need to learn**: Modern CMake, compiler warnings vs. errors, sanitizers (ASan, UBSan), test-driven development setup.
4. **Linux/POSIX APIs involved**: N/A
5. **C++20 concepts involved**: N/A
6. **Components/classes to implement**: `CMakeLists.txt`, dummy test, basic `.gitignore`.
7. **Expected architecture**: Just the directory structure (`src`, `tests`, `benchmarks`, `docs`).
8. **Functional requirements**: Project compiles, tests run and pass, sanitizers can be toggled.
9. **Unit tests to add**: Dummy sanity check.
10. **Integration/manual tests to perform**: Build with/without sanitizers.
11. **Common bugs and failure modes**: CMake syntax errors, incorrect C++ standard propagation, linker errors with GTest.
12. **Performance considerations**: Debug vs. Release builds.
13. **Definition of Done**: `cmake --build build` and `ctest` succeed out of the box.
14. **Suggested Git commit(s)**: `chore: setup cmake project structure`, `test: integrate GoogleTest and sanitizers`
15. **What I should understand**: How CMake targets work, what AddressSanitizer does.
16. **What should explicitly NOT be implemented yet**: Any networking code.
17. **Difficulty and estimated effort**: Difficulty: Easy. Implementation: 1 hr. Learning: 2 hrs.

---

## Phase 1: Basic blocking TCP server
1. **Primary objective**: Create a single-threaded server that accepts one connection, reads data, sends a hardcoded response, and closes.
2. **Why this sprint exists**: Demystifies the core POSIX network API before adding abstractions.
3. **Concepts I need to learn**: Sockets as file descriptors, IP addresses, ports, byte ordering (endianness), the TCP handshake (SYN/ACK).
4. **Linux/POSIX APIs involved**: `socket()`, `bind()`, `listen()`, `accept()`, `recv()`, `send()`, `close()`, `setsockopt(SO_REUSEADDR)`.
5. **C++20 concepts involved**: Basic I/O.
6. **Components/classes to implement**: RAII `Socket` wrapper, `TcpListener`.
7. **Expected architecture**: Client → TCP Server (Blocking) → Response.
8. **Functional requirements**: Can connect via `curl`, receive a "Hello World", and terminate safely.
9. **Unit tests to add**: RAII Socket tests (ensure destruction calls close).
10. **Integration/manual tests to perform**: `curl -v http://localhost:8080`, `nc localhost 8080`.
11. **Common bugs and failure modes**: `EADDRINUSE` (missing `SO_REUSEADDR`), forgetting `htons/htonl`, leaking file descriptors.
12. **Performance considerations**: Blocking `accept()` and `recv()` means the server handles exactly one user at a time.
13. **Definition of Done**: Server runs, accepts a connection, sends valid HTTP string, and doesn't leak FDs on exit.
14. **Suggested Git commit(s)**: `feat: add RAII Socket class`, `feat: implement blocking TCP listener`
15. **What I should understand**: The lifecycle of a POSIX socket, why RAII is mandatory for file descriptors.
16. **What should explicitly NOT be implemented yet**: Non-blocking I/O, epoll, HTTP parsing.
17. **Difficulty and estimated effort**: Difficulty: Medium. Implementation: 2 hrs. Learning: 4 hrs.

---

## Phase 2: HTTP/1.1 request parsing
1. **Primary objective**: Parse raw bytes from a socket into structured HTTP request objects.
2. **Why this sprint exists**: HTTP is a text-based protocol; processing it requires robust string parsing and state management.
3. **Concepts I need to learn**: HTTP/1.1 syntax (RFC 9112), state machines.
4. **Linux/POSIX APIs involved**: N/A (Pure C++ logic).
5. **C++20 concepts involved**: `std::string_view`, `std::optional`, `enum class`.
6. **Components/classes to implement**: `HttpRequest`, `HttpParser` (State Machine: Request Line, Headers, Body).
7. **Expected architecture**: Client → TCP Server → `HttpParser` → Response.
8. **Functional requirements**: Parse Method (GET/POST), URI, Version, Headers, and Content-Length based bodies.
9. **Unit tests to add**: Parse valid requests, reject malformed requests (missing CRLF, invalid methods).
10. **Integration/manual tests to perform**: Send malformed data via `nc`.
11. **Common bugs and failure modes**: Buffer overflows, off-by-one errors with `\r\n`, allocating strings instead of using `string_view`.
12. **Performance considerations**: Excessive string copying (mitigated by `std::string_view`).
13. **Definition of Done**: `HttpParser` correctly parses valid HTTP and returns explicit errors for invalid HTTP.
14. **Suggested Git commit(s)**: `feat: define HttpRequest struct`, `feat: implement HttpParser state machine`, `test: add parser unit tests`
15. **What I should understand**: How to write a zero-copy (or low-copy) text parser.
16. **What should explicitly NOT be implemented yet**: Routing, HTTP bodies (keep to headers initially).
17. **Difficulty and estimated effort**: Difficulty: Hard. Implementation: 4 hrs. Learning: 3 hrs.

---

## Phase 3: HTTP responses and routing
1. **Primary objective**: Map parsed URIs to specific logic handlers and generate properly formatted HTTP responses.
2. **Why this sprint exists**: A web server needs to do more than echo; it needs to serve dynamic content based on the URL.
3. **Concepts I need to learn**: URI paths, HTTP status codes.
4. **Linux/POSIX APIs involved**: N/A.
5. **C++20 concepts involved**: `std::function`, lambdas, `std::unordered_map`.
6. **Components/classes to implement**: `HttpResponse`, `Router`, `RouteHandler`.
7. **Expected architecture**: Client → TCP Server → `HttpParser` → `Router` → `RouteHandler` → `HttpResponse` → TCP Server.
8. **Functional requirements**: Support `/`, `/api/data`, and return 404 for unknown routes.
9. **Unit tests to add**: Router match tests, Response serialization tests.
10. **Integration/manual tests to perform**: Use Postman/curl to hit various routes and check status codes.
11. **Common bugs and failure modes**: Incorrect Content-Length calculation for responses, trailing slash mismatches.
12. **Performance considerations**: Router lookup speed (O(1) with hash maps).
13. **Definition of Done**: Server responds with dynamic payloads and correct status codes based on the requested URI.
14. **Suggested Git commit(s)**: `feat: implement HttpResponse serialization`, `feat: add basic Router mechanism`
15. **What I should understand**: How callback-based routing works in web frameworks.
16. **What should explicitly NOT be implemented yet**: Threading, epoll.
17. **Difficulty and estimated effort**: Difficulty: Medium. Implementation: 3 hrs. Learning: 1 hr.

---

## Phase 4: Non-blocking sockets
1. **Primary objective**: Convert the socket to non-blocking mode to prepare for event-driven I/O.
2. **Why this sprint exists**: A blocking server halts the entire thread while waiting for a slow client. Non-blocking is mandatory for high concurrency.
3. **Concepts I need to learn**: Blocking vs. Non-blocking I/O, `EAGAIN` / `EWOULDBLOCK`.
4. **Linux/POSIX APIs involved**: `fcntl(fd, F_SETFL, O_NONBLOCK)`.
5. **C++20 concepts involved**: N/A.
6. **Components/classes to implement**: Add `set_non_blocking()` to `Socket`.
7. **Expected architecture**: Unchanged, but I/O behavior changes drastically.
8. **Functional requirements**: Read/write operations return immediately if no data is ready, rather than hanging.
9. **Unit tests to add**: N/A (Hard to unit test natively, rely on integration).
10. **Integration/manual tests to perform**: Try reading from a connected socket that hasn't sent data; observe `EAGAIN`.
11. **Common bugs and failure modes**: Treating `EAGAIN` as a fatal error instead of a signal to try again later. CPU spinning in a `while(true)` loop (busy-waiting) because `accept` no longer blocks.
12. **Performance considerations**: Busy-waiting spikes CPU to 100%. We need an event notification system (Phase 5).
13. **Definition of Done**: Sockets are successfully set to non-blocking; `recv` handles `EAGAIN` gracefully.
14. **Suggested Git commit(s)**: `feat: add non-blocking mode to sockets`, `fix: handle EAGAIN in I/O operations`
15. **What I should understand**: Why non-blocking I/O requires a fundamentally different architectural model.
16. **What should explicitly NOT be implemented yet**: epoll.
17. **Difficulty and estimated effort**: Difficulty: Medium. Implementation: 1 hr. Learning: 3 hrs.

---

## Phase 5: Linux epoll event loop
1. **Primary objective**: Implement an `epoll`-based event loop to efficiently monitor thousands of non-blocking sockets.
2. **Why this sprint exists**: Replaces CPU-burning busy-waiting with kernel-level event notification. This is the heart of high-performance Linux servers (like Nginx).
3. **Concepts I need to learn**: Readiness-based I/O, Level-Triggered (LT) vs. Edge-Triggered (ET) events.
4. **Linux/POSIX APIs involved**: `epoll_create1()`, `epoll_ctl()`, `epoll_wait()`.
5. **C++20 concepts involved**: N/A.
6. **Components/classes to implement**: `Epoll`, `EventLoop`.
7. **Expected architecture**: Client → TCP Listener → `epoll` Event Loop → `HttpParser`.
8. **Functional requirements**: Single thread can handle multiple concurrent connections without blocking.
9. **Unit tests to add**: Mock epoll behavior (difficult, usually tested via integration).
10. **Integration/manual tests to perform**: Connect 100 `nc` clients simultaneously; ensure server doesn't crash and handles all when they send data.
11. **Common bugs and failure modes**: Epoll starvation (in ET mode), forgetting to read until `EAGAIN`, managing the lifecycle of the FD in the epoll instance.
12. **Performance considerations**: Drastic improvement in concurrency. 
13. **Definition of Done**: Event loop successfully routes `EPOLLIN` and `EPOLLOUT` events to the correct file descriptors.
14. **Suggested Git commit(s)**: `feat: implement Epoll wrapper`, `feat: create main EventLoop`
15. **What I should understand**: The difference between polling (select/poll) and event notification (epoll).
16. **What should explicitly NOT be implemented yet**: Thread pools.
17. **Difficulty and estimated effort**: Difficulty: Very Hard. Implementation: 5 hrs. Learning: 6 hrs.

---

## Phase 6: Connection management and persistent connections
1. **Primary objective**: Track the state of individual client connections and support HTTP Keep-Alive.
2. **Why this sprint exists**: In an event-driven system, you process partial data. You need a place to store that data between `epoll_wait` events.
3. **Concepts I need to learn**: State machines per connection, HTTP Keep-Alive mechanism.
4. **Linux/POSIX APIs involved**: N/A.
5. **C++20 concepts involved**: `std::shared_ptr`, `std::weak_ptr` (if managing lifecycles), move semantics.
6. **Components/classes to implement**: `Connection`, `ConnectionManager`.
7. **Expected architecture**: `epoll` Event Loop → `ConnectionManager` → `Connection` (holds parser state and read/write buffers).
8. **Functional requirements**: A single TCP connection can serve multiple HTTP requests sequentially.
9. **Unit tests to add**: Connection state transitions.
10. **Integration/manual tests to perform**: `curl -v -H "Connection: keep-alive" http://localhost:8080`.
11. **Common bugs and failure modes**: Memory leaks (connections never destroyed), buffer corruption from mixed requests.
12. **Performance considerations**: Reusing connections saves TCP handshake overhead, massively improving throughput.
13. **Definition of Done**: Server correctly parses `Connection: keep-alive`, keeps socket open, and processes subsequent requests.
14. **Suggested Git commit(s)**: `feat: add Connection class for state tracking`, `feat: implement HTTP keep-alive`
15. **What I should understand**: Why event loops require state to be explicitly managed on the heap rather than the stack.
16. **What should explicitly NOT be implemented yet**: Thread pools.
17. **Difficulty and estimated effort**: Difficulty: Hard. Implementation: 4 hrs. Learning: 2 hrs.

---

## Phase 7: Thread pool and concurrent request processing
1. **Primary objective**: Offload HTTP parsing and route handling to worker threads so the main event loop never blocks.
2. **Why this sprint exists**: A single-threaded event loop bottlenecks on CPU-bound tasks (like parsing or DB queries). A thread pool utilizes multi-core CPUs.
3. **Concepts I need to learn**: Thread synchronization, task queues, race conditions, deadlocks, spurious wakeups.
4. **Linux/POSIX APIs involved**: `eventfd` (to wake up epoll from another thread).
5. **C++20 concepts involved**: `std::thread`, `std::mutex`, `std::condition_variable`, `std::atomic`, `std::function`.
6. **Components/classes to implement**: `ThreadPool`, `TaskQueue`.
7. **Expected architecture**: `epoll` Event Loop → `Connection` → Enqueue Task → `ThreadPool` → `Router` → Event Loop Write.
8. **Functional requirements**: Multiple requests process in parallel.
9. **Unit tests to add**: ThreadPool enqueuing/dequeuing, stress test with math tasks.
10. **Integration/manual tests to perform**: Add a `sleep(1)` route and ensure it doesn't block other requests.
11. **Common bugs and failure modes**: Data races on `Connection` objects, deadlocks in thread pool shutdown, use-after-free if a connection closes while a worker is processing it.
12. **Performance considerations**: Thread creation overhead is eliminated; lock contention on the task queue must be minimized.
13. **Definition of Done**: Heavy requests are processed by workers; event loop remains responsive.
14. **Suggested Git commit(s)**: `feat: implement ThreadPool and TaskQueue`, `feat: integrate ThreadPool with EventLoop`
15. **What I should understand**: The Reactor pattern (Event Loop + Thread Pool).
16. **What should explicitly NOT be implemented yet**: Lock-free queues.
17. **Difficulty and estimated effort**: Difficulty: Very Hard. Implementation: 6 hrs. Learning: 6 hrs.

---

## Phase 8: Static file serving and filesystem security
1. **Primary objective**: Read files from disk and send them to clients safely.
2. **Why this sprint exists**: A web server needs to serve HTML, CSS, and JS. Doing this securely requires sanitizing inputs.
3. **Concepts I need to learn**: Path traversal attacks (`../`), MIME types.
4. **Linux/POSIX APIs involved**: `open()`, `read()`, `fstat()`.
5. **C++20 concepts involved**: `std::filesystem`.
6. **Components/classes to implement**: `StaticFileHandler`, `MimeTypeDetector`.
7. **Expected architecture**: `Router` → `StaticFileHandler` → Disk.
8. **Functional requirements**: Serves files from a defined `document_root`, correctly sets `Content-Type`.
9. **Unit tests to add**: Path normalization, malicious path rejection.
10. **Integration/manual tests to perform**: Try `curl http://localhost/../../etc/passwd`.
11. **Common bugs and failure modes**: Security vulnerabilities (directory traversal), reading large files entirely into RAM (OOM).
12. **Performance considerations**: `read`/`write` involves user-space copying. (Optimization: `sendfile()` comes later).
13. **Definition of Done**: Server securely serves static assets with correct MIME types.
14. **Suggested Git commit(s)**: `feat: add StaticFileHandler`, `sec: prevent directory traversal attacks`
15. **What I should understand**: The security implications of exposing the filesystem to the network.
16. **What should explicitly NOT be implemented yet**: `sendfile()` zero-copy optimization.
17. **Difficulty and estimated effort**: Difficulty: Medium. Implementation: 3 hrs. Learning: 2 hrs.

---

## Phase 9: Request limits, timeouts, error handling and robustness
1. **Primary objective**: Protect the server from misbehaving clients and resource exhaustion (Slowloris attacks).
2. **Why this sprint exists**: Real-world clients are slow, drop connections, and send malicious data.
3. **Concepts I need to learn**: Timers in event loops, connection eviction.
4. **Linux/POSIX APIs involved**: `timerfd` (optional, for epoll timer integration).
5. **C++20 concepts involved**: N/A.
6. **Components/classes to implement**: `TimerQueue`, max header/body limit checks.
7. **Expected architecture**: `EventLoop` manages timeouts and evicts stale `Connection` objects.
8. **Functional requirements**: Server drops clients that take > 30s to send a request, rejects payloads > 10MB.
9. **Unit tests to add**: Header size validation, body size validation.
10. **Integration/manual tests to perform**: Open a socket and send 1 byte every 10 seconds (Slowloris).
11. **Common bugs and failure modes**: Memory leaks when evicting connections, race conditions if a timer fires while a worker is processing the connection.
12. **Performance considerations**: Timer management can be O(N); a min-heap or timing wheel is needed for O(1) or O(log N).
13. **Definition of Done**: Server survives Slowloris and oversized payload attacks.
14. **Suggested Git commit(s)**: `feat: add connection timeouts`, `feat: enforce max HTTP payload sizes`
15. **What I should understand**: Defensive programming in systems architecture.
16. **What should explicitly NOT be implemented yet**: Complex rate limiting algorithms (Token Bucket).
17. **Difficulty and estimated effort**: Difficulty: Hard. Implementation: 5 hrs. Learning: 3 hrs.

---

## Phase 10: Logging and configuration
1. **Primary objective**: Add structured logging and external configuration parsing.
2. **Why this sprint exists**: Hardcoded ports and silent failures are unacceptable in production.
3. **Concepts I need to learn**: Thread-safe logging, JSON/INI parsing.
4. **Linux/POSIX APIs involved**: N/A.
5. **C++20 concepts involved**: `std::format` (if supported by compiler), thread-local storage.
6. **Components/classes to implement**: `Logger`, `ConfigParser`.
7. **Expected architecture**: All components receive configuration at startup; write to a unified thread-safe logger.
8. **Functional requirements**: Read port, worker count, and doc root from `config.json`. Log requests in Apache/Nginx format.
9. **Unit tests to add**: Logger thread safety, Config parser logic.
10. **Integration/manual tests to perform**: Change config, restart server, verify behavior.
11. **Common bugs and failure modes**: Logger bottlenecking the thread pool (lock contention on disk writes).
12. **Performance considerations**: Synchronous I/O logging can stall the event loop.
13. **Definition of Done**: Server is fully configurable via file and logs all accesses and errors thread-safely.
14. **Suggested Git commit(s)**: `feat: add thread-safe Logger`, `feat: implement ConfigParser`
15. **What I should understand**: The performance cost of I/O in worker threads.
16. **What should explicitly NOT be implemented yet**: Asynchronous/lock-free logging.
17. **Difficulty and estimated effort**: Difficulty: Easy. Implementation: 2 hrs. Learning: 1 hr.

---

## Phase 11: Graceful shutdown and production engineering
1. **Primary objective**: Ensure the server shuts down cleanly, finishing in-flight requests and releasing all resources.
2. **Why this sprint exists**: `Ctrl+C` shouldn't instantly kill active downloads or corrupt state.
3. **Concepts I need to learn**: POSIX signals, atomic flags.
4. **Linux/POSIX APIs involved**: `sigaction()`, `signalfd()` (optional for epoll).
5. **C++20 concepts involved**: `std::atomic<bool>`.
6. **Components/classes to implement**: `SignalHandler`.
7. **Expected architecture**: `SignalHandler` notifies `EventLoop`, which stops accepting connections and waits for `ThreadPool` to drain.
8. **Functional requirements**: `SIGINT` (Ctrl+C) causes server to stop listening, finish current requests, and exit with code 0.
9. **Unit tests to add**: Thread pool drain logic.
10. **Integration/manual tests to perform**: Start a slow download, press Ctrl+C, ensure download finishes before server exits.
11. **Common bugs and failure modes**: Using non-async-signal-safe functions (like `malloc` or `printf`) inside a standard signal handler.
12. **Performance considerations**: N/A.
13. **Definition of Done**: Zero memory leaks reported by ASan on exit, even with active connections.
14. **Suggested Git commit(s)**: `feat: implement graceful shutdown via SIGINT`
15. **What I should understand**: POSIX signal handling constraints.
16. **What should explicitly NOT be implemented yet**: Daemonization.
17. **Difficulty and estimated effort**: Difficulty: Medium. Implementation: 3 hrs. Learning: 3 hrs.

---

## Phase 12: Unit, integration and concurrency testing
1. **Primary objective**: Solidify the test suite before optimizing.
2. **Why this sprint exists**: You cannot optimize what you cannot verify. Optimization often breaks edge cases.
3. **Concepts I need to learn**: ThreadSanitizer (TSan), fuzzing (basic).
4. **Linux/POSIX APIs involved**: N/A.
5. **C++20 concepts involved**: N/A.
6. **Components/classes to implement**: Python/Bash integration test scripts.
7. **Expected architecture**: N/A.
8. **Functional requirements**: 90%+ code coverage on core logic.
9. **Unit tests to add**: Exhaustive parser edge cases.
10. **Integration/manual tests to perform**: Run test suite with TSan enabled to catch data races.
11. **Common bugs and failure modes**: Flaky concurrency tests.
12. **Performance considerations**: N/A.
13. **Definition of Done**: Automated suite can reliably prove the server works under normal and hostile conditions.
14. **Suggested Git commit(s)**: `test: add comprehensive integration suite`, `test: enable ThreadSanitizer`
15. **What I should understand**: How to write deterministic tests for non-deterministic asynchronous systems.
16. **What should explicitly NOT be implemented yet**: Optimization.
17. **Difficulty and estimated effort**: Difficulty: Medium. Implementation: 4 hrs. Learning: 2 hrs.

---

## Phase 13: Benchmarking and profiling
1. **Primary objective**: Establish a performance baseline using standard tools.
2. **Why this sprint exists**: "Premature optimization is the root of all evil." We must measure first.
3. **Concepts I need to learn**: Load testing, latency percentiles, flame graphs.
4. **Linux/POSIX APIs involved**: `perf`.
5. **C++20 concepts involved**: N/A.
6. **Components/classes to implement**: Google Benchmark targets for string parsing.
7. **Expected architecture**: N/A.
8. **Functional requirements**: Document maximum Req/Sec, P99 latency, and identify the #1 CPU bottleneck.
9. **Unit tests to add**: Microbenchmarks for `HttpParser`.
10. **Integration/manual tests to perform**: Run `wrk -t4 -c100 -d30s http://localhost:8080`. Profile with `perf record` and generate a flame graph.
11. **Common bugs and failure modes**: Benchmarking the network (e.g., localhost loopback limits) instead of the server CPU.
12. **Performance considerations**: This sprint *is* performance consideration.
13. **Definition of Done**: A written baseline report exists detailing current metrics and identified bottlenecks.
14. **Suggested Git commit(s)**: `bench: add Google Benchmark targets`, `docs: publish initial performance baseline`
15. **What I should understand**: How to read a flame graph and differentiate between I/O bound and CPU bound workloads.
16. **What should explicitly NOT be implemented yet**: Fixing the bottlenecks.
17. **Difficulty and estimated effort**: Difficulty: Medium. Implementation: 2 hrs. Learning: 4 hrs.

---

## Phase 14: Performance optimization
1. **Primary objective**: Implement targeted optimizations based on Phase 13 profiles.
2. **Why this sprint exists**: To push the server to "production-quality, high-performance" status.
3. **Concepts I need to learn**: Zero-copy network I/O, lock-free data structures, custom allocators.
4. **Linux/POSIX APIs involved**: `sendfile()`, `TCP_NODELAY`.
5. **C++20 concepts involved**: Memory arenas / custom allocators (optional).
6. **Components/classes to implement**: `sendfile` integration in `StaticFileHandler`.
7. **Expected architecture**: Replaces user-space file buffering with kernel-space zero-copy.
8. **Functional requirements**: 20%+ increase in throughput for static files.
9. **Unit tests to add**: Ensure `sendfile` falls back to `read`/`write` if it fails.
10. **Integration/manual tests to perform**: Re-run `wrk` and compare to baseline.
11. **Common bugs and failure modes**: `sendfile` blocking the event loop if not used with non-blocking sockets correctly.
12. **Performance considerations**: Massive reduction in CPU context switches and memory bandwidth usage.
13. **Definition of Done**: Benchmarks prove statistically significant improvement.
14. **Suggested Git commit(s)**: `perf: replace read/write with sendfile for static assets`, `perf: disable Nagle's algorithm (TCP_NODELAY)`
15. **What I should understand**: The cost of crossing the kernel/user-space boundary.
16. **What should explicitly NOT be implemented yet**: Over-engineering for 1% gains.
17. **Difficulty and estimated effort**: Difficulty: Hard. Implementation: 5 hrs. Learning: 4 hrs.

---

## Phase 15: Final architecture cleanup, documentation and portfolio preparation
1. **Primary objective**: Polish the codebase, ensure RAII consistency, and write a stellar README.
2. **Why this sprint exists**: A portfolio project is only as good as its presentation and maintainability.
3. **Concepts I need to learn**: Technical writing, architecture diagramming.
4. **Linux/POSIX APIs involved**: N/A.
5. **C++20 concepts involved**: `const` correctness review, `noexcept` audits.
6. **Components/classes to implement**: README, Architecture Markdown.
7. **Expected architecture**: Cleanest possible iteration of the Event Loop + Thread Pool design.
8. **Functional requirements**: Code passes strict linting (`clang-tidy`).
9. **Unit tests to add**: Ensure 100% pass rate in CI.
10. **Integration/manual tests to perform**: End-to-end smoke test.
11. **Common bugs and failure modes**: Broken markdown links, inaccurate documentation.
12. **Performance considerations**: N/A.
13. **Definition of Done**: Repository is public-ready with clear build instructions and performance claims backed by methodology.
14. **Suggested Git commit(s)**: `docs: finalize README and architecture diagrams`, `style: apply clang-format and fix tidy warnings`
15. **What I should understand**: How to present complex engineering work to technical recruiters/peers.
16. **What should explicitly NOT be implemented yet**: New features.
17. **Difficulty and estimated effort**: Difficulty: Easy. Implementation: 3 hrs. Learning: 1 hr.

---

## Checklists

### 1. Feature Checklist
- [ ] IPv4 TCP Socket Server
- [ ] Non-blocking I/O
- [ ] epoll Event Loop
- [ ] Thread Pool
- [ ] HTTP/1.1 Request Parsing
- [ ] GET, POST, HEAD support
- [ ] HTTP Headers & Body Handling
- [ ] Keep-Alive / Persistent Connections
- [ ] Routing & Query Parameters
- [ ] Static File Serving (with `sendfile`)
- [ ] MIME Type Detection
- [ ] Connection Timeouts
- [ ] Graceful Shutdown
- [ ] Structured Logging
- [ ] JSON/INI Configuration

### 2. Concepts Checklist
- [ ] File descriptors and Everything-is-a-file
- [ ] TCP Handshake & Endianness
- [ ] Blocking vs. Non-blocking I/O
- [ ] Readiness-based I/O (epoll, level/edge triggered)
- [ ] State Machines (Parsers)
- [ ] Reactor Pattern
- [ ] Multi-threading & Synchronization
- [ ] Race conditions, Deadlocks, Spurious Wakeups
- [ ] Zero-copy I/O
- [ ] Context switching overhead

### 3. Linux API Checklist
- [ ] `socket`, `bind`, `listen`, `accept`
- [ ] `recv`, `send`, `close`
- [ ] `setsockopt` (`SO_REUSEADDR`, `TCP_NODELAY`)
- [ ] `fcntl` (`O_NONBLOCK`)
- [ ] `epoll_create1`, `epoll_ctl`, `epoll_wait`
- [ ] `open`, `read`, `fstat`
- [ ] `sendfile`
- [ ] `sigaction` (Signal handling)

### 4. C++20 Checklist
- [ ] RAII & Rule of Zero/Five
- [ ] `std::string_view` (Zero-copy parsing)
- [ ] `std::thread`, `std::mutex`, `std::condition_variable`
- [ ] `std::atomic`
- [ ] `std::function`, Lambdas
- [ ] `std::filesystem`
- [ ] `std::shared_ptr` / `std::unique_ptr`
- [ ] `const` correctness & `noexcept`

### 5. Testing Checklist
- [ ] GoogleTest Unit Tests
- [ ] Parser edge cases (Malformed HTTP)
- [ ] Integration script (cURL/nc)
- [ ] ThreadSanitizer (TSan) runs clean
- [ ] AddressSanitizer (ASan) runs clean
- [ ] UndefinedBehaviorSanitizer (UBSan) runs clean

### 6. Benchmark Checklist
- [ ] Baseline Blocking Server (Req/Sec, Latency)
- [ ] Baseline epoll Server (Single thread)
- [ ] epoll + Thread Pool Server
- [ ] `sendfile` vs `read/write` Static Serving
- [ ] Documented test machine specs and tool used (`wrk`)

### 7. Git Milestone Checklist
- [ ] Phase 0-3: Core parsing and blocking I/O committed.
- [ ] Phase 4-6: Asynchronous core (epoll) committed.
- [ ] Phase 7: Concurrency (Thread pool) committed.
- [ ] Phase 8-11: Production features committed.
- [ ] Phase 12-15: Tests, Profiling, and Docs committed.

### 8. Final Resume-Ready Project Description
> **High-Performance HTTP/1.1 Server in C++20**
> Engineered a custom, production-ready web server from scratch on Linux without standard networking frameworks. Implemented a non-blocking, event-driven architecture utilizing `epoll` and the Reactor pattern to handle high concurrency. Developed a custom thread pool for asynchronous request processing and a zero-copy HTTP/1.1 parser using `std::string_view`. Optimized static file delivery via kernel-level `sendfile()`, achieving [X] requests/second and [Y]ms p99 latency under heavy load. Enforced rigorous memory safety and thread safety using RAII principles and Clang sanitizers.
