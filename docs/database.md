# Database & C++20 Coroutines

Orbit provides seamless integration with PostgreSQL using raw asynchronous networking (`io_uring`/`epoll`) bridged perfectly into **C++20 Coroutines** via `Task<T>`.

This allows you to write non-blocking database queries exactly like Node.js or Python without spanning unnecessary OS threads!

## Setup

Include the necessary headers:
```cpp
#include "database/PostgresClient.hpp"
#include "database/PostgresCoro.hpp"
#include "concurrency/Task.hpp"
```

## Awaiting Database Queries

Define a handler that returns `concurrency::Task` instead of `void`. You can then use the `co_await` keyword for database operations.

```cpp
using namespace database;
using namespace concurrency;

Task db_handler(HttpRequest& req, std::shared_ptr<ResponseWriter> writer) {
    // Initialize PostgresClient using the active Proactor event loop
    auto pg = std::make_shared<PostgresClient>(&writer->proactor(), "dbname=postgres");
    
    // 1. Asynchronously await database connection
    bool connected = co_await connect_async(pg);
    if (!connected) {
        writer->send(HttpResponse().status(HttpStatus::InternalServerError).send("DB Failed"));
        co_return; 
    }

    // 2. Asynchronously await query results without blocking the server
    PGresult* res = co_await query_async(pg, "SELECT current_timestamp;");
    if (res) {
        std::string ts = PQgetvalue(res, 0, 0);
        writer->send(HttpResponse().status(HttpStatus::OK).send("DB Time: " + ts));
        PQclear(res);
    }
}
```

## Hooking into Router

Simply pass your Coroutine handler to the standard `app.get()` router:

```cpp
app.get("/db", [](HttpRequest& req, std::shared_ptr<ResponseWriter> res) {
    db_handler(req, res); // Starts the coroutine Task asynchronously
});
```
