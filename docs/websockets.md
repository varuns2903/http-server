# WebSockets

Orbit has built-in support for RFC 6455 WebSockets. Because the core engine is asynchronous, a single server instance can hold millions of concurrent WebSocket connections efficiently without exhausting OS threads.

## Defining a WebSocket Route

Instead of `app.get()`, use `app.ws()` and pass a handler that accepts a `WebSocketConnection&`.

```cpp
app.ws("/chat", [](http::websocket::WebSocketConnection& ws) {
    
    // Register message callback
    ws.on_message([&ws](const std::string& msg, bool is_binary) {
        std::cout << "Received: " << msg << std::endl;
        
        // Echo it back
        ws.send("Echo: " + msg, is_binary);
    });

    // Register close callback
    ws.on_close([]() {
        std::cout << "Client disconnected" << std::endl;
    });
    
});
```

## Broadcasting

To build a chat server, you can store `WebSocketConnection` references or broadcast messages globally. The underlying sockets are non-blocking, making `send()` extremely fast.
