# 🚀 Orbit: C++20 High-Performance HTTP Framework (Interview Prep)

This document is your comprehensive cheat sheet for discussing your C++ HTTP server project (**Orbit**) in interviews. It breaks down the architecture, design choices, and deeply technical Q&A you can expect from senior engineers.

---

## 1. Project Overview & Pitch
**The Pitch**: "I built Orbit, a high-performance, asynchronous HTTP and WebSocket framework in C++20 from scratch. It leverages modern Linux kernel networking features like `io_uring` (and falls back to `epoll`) to achieve non-blocking I/O. I implemented a robust Proactor event loop, a multi-threaded worker pool to offload CPU-bound tasks, Express.js-style dynamic routing, and a composable middleware architecture supporting Redis-backed distributed sessions and Token-Bucket rate limiting."

## 2. Core Architecture

### 2.1 The Proactor Pattern
Orbit uses the **Proactor** pattern (unlike Node.js or Nginx which use the Reactor pattern).
- **Reactor (epoll)**: The kernel tells you *when* a socket is ready to be read from or written to. You then perform the `recv()` or `send()` synchronously (which blocks briefly).
- **Proactor (io_uring)**: You tell the kernel *what* to do (e.g., "Read 1024 bytes from FD 5 into this buffer"). The kernel does the operation asynchronously in the background. The kernel then notifies you when the operation is *completely finished*.

### 2.2 Event Loop & Connection Manager
- **EventLoop**: The single-threaded heart of the server. It waits for I/O completion events from `io_uring` or `epoll` and dispatches them to the Connection Manager. It never performs heavy computation.
- **ConnectionManager**: Tracks the lifecycle of every active TCP connection. It manages idle timeouts, HTTP Keep-Alive, and cleans up dead connections.
- **Connection State Machine**: Each connection is a state machine moving through: `READING_HEADERS` -> `READING_BODY` -> `PROCESSING` -> `WRITING_RESPONSE` -> `CLOSING/KEEP_ALIVE`.

### 2.3 Thread Pool Offloading
Because the event loop is single-threaded, any heavy computation would block it. Once a `Connection` fully reads an HTTP request into memory, the `Connection` pushes the request into a **Thread Pool** queue. A worker thread picks it up, parses the HTTP, runs the middleware, executes the route handler, serializes the response, and then tells the event loop to asynchronously write the response back to the socket.

### 2.4 Middleware & Routing
- **Middleware**: Uses the Chain of Responsibility pattern. Requests pass through Middlewares (CORS, Rate Limiting, Sessions) before hitting the route. If a middleware returns `false`, the chain breaks (e.g., Rate Limiter rejects request).
- **Router**: Supports exact matching and dynamic parameters (e.g., `/users/:id`).

---

## 3. Interview Q&A

### Q1: Why did you use `io_uring` instead of `epoll`? What is the difference?
**Answer**: 
`epoll` is a readiness-notification model. You call `epoll_wait`, which tells you a socket is ready, and then you issue a `recv()` syscall. 
`io_uring` is a true asynchronous completion model. It uses two shared memory ring buffers (Submission Queue and Completion Queue) between user space and kernel space. This means I can submit hundreds of `recv()` and `send()` operations without making a single system call, avoiding expensive user-to-kernel context switches. It also offloads blocking operations to kernel worker threads (`io-wq`), providing significantly higher throughput and lower latency under heavy load.

### Q2: How did you implement Rate Limiting?
**Answer**:
I implemented the **Token Bucket algorithm**. The rate limiter takes two parameters: `max_requests` (bucket capacity) and `window` (refill rate).
Each client IP maps to a bucket. When a request comes in, we calculate the elapsed time since the last refill and add new tokens proportionally. If tokens > 0, we decrement and allow the request. If tokens == 0, we reject with `429 Too Many Requests`. To make it thread-safe, I protected the bucket map with a `std::mutex`, allowing safe access from the thread pool.

### Q3: How do you handle HTTP Keep-Alive and prevent resource exhaustion?
**Answer**:
Every connection is tracked by the `ConnectionManager`. When a connection finishes a request, if the headers contain `Connection: keep-alive`, the connection state resets and waits for more data rather than closing the socket. To prevent clients from holding connections open indefinitely, the ConnectionManager runs a periodic sweep that checks the `last_activity` timestamp of every connection. If a connection exceeds the idle timeout (e.g., 10 seconds), it forcefully closes the socket and reclaims the memory.

### Q4: How did you implement TLS/SSL?
**Answer**:
Integrating OpenSSL with an asynchronous event loop is challenging because OpenSSL expects blocking sockets. To solve this, I used **OpenSSL Memory BIOs (Basic Input/Output)**. 
Instead of OpenSSL reading directly from the socket, my event loop reads encrypted bytes from the socket asynchronously and pushes them into an in-memory buffer (the `rbio`). I then tell OpenSSL to decrypt data from that memory buffer. Conversely, when writing, OpenSSL encrypts data into a `wbio` memory buffer, and my event loop asynchronously flushes that buffer to the socket.

### Q5: How do you handle memory management and prevent memory leaks?
**Answer**:
I relied heavily on Modern C++ RAII (Resource Acquisition Is Initialization) and smart pointers.
- Sockets and file descriptors are managed by RAII wrappers that automatically `close(fd)` in their destructor.
- `Connection` objects are managed using `std::shared_ptr`. The event loop and the thread pool hold shared pointers to the connection. Once the connection is removed from the manager and the thread pool finishes processing it, the reference count drops to 0 and it cleans itself up safely.

### Q6: How does the Thread Pool work?
**Answer**:
The thread pool consists of a fixed number of `std::thread` workers (typically matching `std::thread::hardware_concurrency()`). They sleep on a `std::condition_variable`. When the event loop pushes a new parsing/handling task into a `std::queue`, it calls `notify_one()`. An idle thread wakes up, locks the queue via `std::mutex`, pops the task, releases the lock, and executes the HTTP handler. This guarantees the single-threaded event loop is never blocked by business logic.

### Q7: How do you serve static files efficiently?
**Answer**:
Instead of reading a file into user-space memory and then writing it to the socket (which involves two context switches and copying data), I use the Linux `sendfile()` system call (or its `io_uring` equivalent). `sendfile` triggers a zero-copy transfer entirely within the kernel, reading directly from the filesystem cache and pushing directly to the network socket buffer.

### Q8: Describe your Session Management implementation.
**Answer**:
I implemented a distributed session manager using **Redis**. When a user logs in, I generate a cryptographically secure UUID and set it as a cookie (`Set-Cookie: session_id=...`). The middleware extracts this cookie, connects to Redis using the `hiredis` library, and fetches the user's state. Because it uses Redis, the architecture is stateless—I can spin up 10 instances of Orbit behind a load balancer, and they will all share the same session data securely.

### Q9: What happens if a client sends a 10 GB payload in an HTTP POST?
**Answer**:
To prevent OOM (Out of Memory) crashes, the `Connection` state machine tracks the `Content-Length` header. As we asynchronously read chunks from the socket into our `read_buffer_`, we constantly compare the buffer size against a configured `MAX_BODY_SIZE`. If the payload exceeds the limit, we instantly close the connection and drop the buffers to protect the server's memory.

### Q10: What challenges did you face and how did you overcome them?
**Answer**:
*Use the recent io_uring bug as an example!*
"While integrating `io_uring`, I ran into a subtle bug where the server would occasionally drop connections under load without a response. Using `strace` and deep debugging, I realized that I was using `SOCK_NONBLOCK` on the `accept` syscall but forgetting to pass the `IOSQE_ASYNC` flag to `io_uring`. This caused the kernel to occasionally return `-EAGAIN` inside the io_uring submission, terminating the read cycle prematurely. By strictly enforcing `SOCK_CLOEXEC` and utilizing the `IOSQE_ASYNC` flag, I ensured the kernel properly offloaded blocking operations to the `io-wq` thread pool, completely resolving the dropped connections."
