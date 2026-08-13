#pragma once
#include <string>
#include <memory>
#include <coroutine>
#include <vector>
#include <stdexcept>
#include <atomic>
#include <mongoc/mongoc.h>
#include <bson/bson.h>
#include <orbit/concurrency/ThreadPool.hpp>

namespace database {

class MongoClient {
public:
    struct Config {
        std::string uri{"mongodb://localhost:27017"};
        std::string dbname;
        std::string collection_name;
    };

    struct QueryResult {
        std::vector<std::string> documents;
    };

    MongoClient(concurrency::ThreadPool& thread_pool, const Config& config);
    ~MongoClient();

    MongoClient(const MongoClient&) = delete;
    MongoClient& operator=(const MongoClient&) = delete;

    struct QueryAwaiter {
        MongoClient& client;
        std::string query_json;
        QueryResult result;
        std::string error_msg;
        std::coroutine_handle<> coro;

        bool await_ready() const { return false; }
        void await_suspend(std::coroutine_handle<> h);
        QueryResult await_resume();
    };
    
    struct InsertAwaiter {
        MongoClient& client;
        std::string doc_json;
        bool success{false};
        std::string error_msg;
        std::coroutine_handle<> coro;

        bool await_ready() const { return false; }
        void await_suspend(std::coroutine_handle<> h);
        bool await_resume();
    };

    QueryAwaiter find_async(std::string query_json);
    InsertAwaiter insert_async(std::string doc_json);

    concurrency::ThreadPool& get_thread_pool() { return thread_pool_; }
    mongoc_client_pool_t* get_pool() { return pool_; }
    const Config& get_config() const { return config_; }

private:
    concurrency::ThreadPool& thread_pool_;
    Config config_;
    mongoc_uri_t* uri_{nullptr};
    mongoc_client_pool_t* pool_{nullptr};
    
    static std::atomic<int> init_count_;
};

} // namespace database
