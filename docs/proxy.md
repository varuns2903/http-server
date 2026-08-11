# API Gateway, Proxy & Load Balancing

Orbit is not just a web framework; it also acts as a high-performance Reverse Proxy and Load Balancer. It supports HTTP connection pooling and TLS Session Reuse for ultra-low latency routing.

## Reverse Proxy

You can proxy any path to an upstream server. Orbit automatically handles chunked transfer encodings, headers (`X-Forwarded-For`), and Keep-Alive multiplexing.

```cpp
#include "middleware/Proxy.hpp"

// Proxy all /api requests to an upstream backend
middleware::ProxyOptions proxy_opts;
proxy_opts.strip_prefix = true; // strips "/api" before forwarding

app.use("/api", middleware::Proxy::create("http://localhost:8081", proxy_opts));
```

## Load Balancing

For distributing traffic across multiple backend servers, use the `LoadBalancer` middleware. It supports connection pooling and TLS multiplexing out of the box.

```cpp
middleware::LoadBalancerOptions lb_opts;
lb_opts.strip_prefix = true;

auto lb = middleware::LoadBalancer::create({
    "http://localhost:8081",
    "http://localhost:8082",
    "https://api.secure-backend.com" // TLS Session Reuse is natively supported!
}, lb_opts);

app.use("/lb-api", lb);
```

## Connection Pooling (Zero-Overhead)

Orbit maintains a global `ConnectionPool` mapped by `Host:Port`. When a Proxy or Load Balancer receives a request, it grabs a dormant TCP or TLS connection instead of performing a new Handshake. If no connections are available, it spawns a new one non-blockingly via the `Proactor`.
