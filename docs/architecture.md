# Architecture Document

*(This document will evolve as we progress through the sprints.)*

## Planned Architecture

The server aims for a modular architecture separating network I/O, HTTP parsing, routing, and concurrency.

### Core Modules
* **Network**: Handles POSIX sockets, epoll event loop, and non-blocking I/O.
* **HTTP**: State machine for parsing HTTP/1.1 requests and generating responses.
* **Routing**: Maps URLs to specific handlers or static files.
* **Concurrency**: Thread pool for handling worker tasks without blocking the main event loop.

### Design Principles
* RAII everywhere (Rule of Zero where possible).
* Minimal global state.
* Explicit ownership (no raw owning pointers).
* Deterministic error handling (checking all system call returns).
