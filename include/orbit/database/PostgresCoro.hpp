#pragma once
#include <orbit/database/PostgresClient.hpp>
#include <coroutine>
#include <memory>
#include <string>

namespace database {

struct ConnectAwaiter {
    std::shared_ptr<PostgresClient> client;
    bool success = false;

    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) {
        client->connect([this, h](bool s) {
            success = s;
            h.resume();
        });
    }
    bool await_resume() { return success; }
};

inline ConnectAwaiter connect_async(std::shared_ptr<PostgresClient> client) {
    return ConnectAwaiter{std::move(client)};
}

struct QueryAwaiter {
    std::shared_ptr<PostgresClient> client;
    std::string sql;
    PGresult* result = nullptr;

    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) {
        client->query(sql, [this, h](PGresult* r) {
            result = r;
            h.resume();
        });
    }
    PGresult* await_resume() { return result; }
};

inline QueryAwaiter query_async(std::shared_ptr<PostgresClient> client, const std::string& sql) {
    return QueryAwaiter{std::move(client), sql};
}

} // namespace database
