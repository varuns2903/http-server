# Routing in Orbit

Orbit uses a fast radix-trie and hash-map based router that executes in O(1) for static routes and O(log N) for dynamic routes. The syntax is heavily inspired by Express.js.

## Basic Routing

```cpp
app.get("/hello", [](HttpRequest& req, std::shared_ptr<ResponseWriter> res) {
    res->send(HttpResponse().send("Hello!"));
});

app.post("/submit", [](HttpRequest& req, std::shared_ptr<ResponseWriter> res) {
    res->send(HttpResponse().send("Submitted!"));
});
```

## Dynamic Parameters

You can define URL parameters using the `:` prefix. They are extracted and made available in `req.params`.

```cpp
app.get("/users/:id", [](HttpRequest& req, std::shared_ptr<ResponseWriter> res) {
    std::string user_id = req.params["id"];
    res->send(HttpResponse().send("User ID: " + user_id));
});
```

## Route Grouping

To group API endpoints under a common prefix, use `app.group()`. This is useful for versioning your APIs.

```cpp
auto api_v1 = app.group("/api/v1");

api_v1->get("/status", [](HttpRequest& req, std::shared_ptr<ResponseWriter> res) {
    res->send(HttpResponse().send("v1 Status OK"));
});

api_v1->post("/login", [](HttpRequest& req, std::shared_ptr<ResponseWriter> res) {
    // Login logic
});
```

## Streaming Responses (Chunked)

Orbit natively supports chunked transfer encoding for streaming large amounts of data without buffering it entirely in memory:

```cpp
app.get("/stream", [](HttpRequest& req, std::shared_ptr<ResponseWriter> res) {
    HttpResponse response;
    response.headers["Transfer-Encoding"] = "chunked";
    res->send_headers(response);
    
    res->write_chunk("This is chunk 1\n");
    res->write_chunk("This is chunk 2\n");
    res->end(); // Important: Sends the 0\r\n\r\n terminator
});
```
