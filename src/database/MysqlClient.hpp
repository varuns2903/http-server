#pragma once
#include <mysql.h>
#include <string>
#include <memory>
#include <coroutine>
#include <vector>
#include <stdexcept>
#include <functional>
#include "../network/Proactor.hpp"

namespace database {

class MysqlClient {
public:
    struct Config {
        std::string host;
        int port{3306};
        std::string user;
        std::string password;
        std::string dbname;
    };

    struct Row {
        std::vector<std::string> columns;
    };

    struct QueryResult {
        std::vector<Row> rows;
        uint64_t affected_rows{0};
    };

    MysqlClient(network::Proactor& proactor, const Config& config);
    ~MysqlClient();

    // Delete copy semantics
    MysqlClient(const MysqlClient&) = delete;
    MysqlClient& operator=(const MysqlClient&) = delete;

    // Awaiter for connecting
    struct ConnectAwaiter {
        MysqlClient& client;
        MYSQL* ret_ptr{nullptr};
        int status{0};
        std::coroutine_handle<> coro;

        bool await_ready() const { return false; }
        void await_suspend(std::coroutine_handle<> h);
        void await_resume();
        void resume_loop();
    };

    // Awaiter for executing a query
    struct QueryAwaiter {
        MysqlClient& client;
        std::string query;
        int err{0};
        int status{0};
        std::coroutine_handle<> coro;
        QueryResult result;

        bool await_ready() const { return false; }
        void await_suspend(std::coroutine_handle<> h);
        QueryResult await_resume();
        void resume_loop();
        void process_result_start();
        void process_result_cont();
        
        MYSQL_RES* res{nullptr};
    };

    ConnectAwaiter connect_async();
    QueryAwaiter query_async(std::string query);

    void close();

    network::Proactor& get_proactor() { return proactor_; }
    MYSQL* get_mysql() { return mysql_; }

private:
    network::Proactor& proactor_;
    Config config_;
    MYSQL* mysql_{nullptr};
    bool connected_{false};

    void wait_for_status(int status, std::function<void()> callback);
};

} // namespace database
