#include <orbit/database/PostgresClient.hpp>
#include <iostream>

namespace database {

PostgresClient::PostgresClient(network::Proactor* proactor, const std::string& conninfo)
    : proactor_(proactor), conninfo_(conninfo) {}

PostgresClient::~PostgresClient() {
    if (conn_) {
        proactor_->remove(PQsocket(conn_));
        PQfinish(conn_);
    }
}

void PostgresClient::connect(std::function<void(bool)> callback) {
    conn_ = PQconnectStart(conninfo_.c_str());
    if (PQstatus(conn_) == CONNECTION_BAD) {
        callback(false);
        return;
    }
    
    PQsetnonblocking(conn_, 1);
    handle_connect(std::move(callback));
}

void PostgresClient::handle_connect(std::function<void(bool)> callback) {
    PostgresPollingStatusType status = PQconnectPoll(conn_);
    int fd = PQsocket(conn_);

    auto self = shared_from_this();
    if (status == PGRES_POLLING_READING) {
        proactor_->async_wait_read(fd, [self, cb = std::move(callback)]() {
            self->handle_connect(cb);
        });
    } else if (status == PGRES_POLLING_WRITING) {
        proactor_->async_wait_write(fd, [self, cb = std::move(callback)]() {
            self->handle_connect(cb);
        });
    } else if (status == PGRES_POLLING_OK) {
        connected_ = true;
        callback(true);
    } else if (status == PGRES_POLLING_FAILED) {
        callback(false);
    }
}

void PostgresClient::query(const std::string& sql, std::function<void(PGresult*)> callback) {
    if (!connected_) {
        callback(nullptr);
        return;
    }

    if (PQsendQuery(conn_, sql.c_str()) == 0) {
        callback(nullptr);
        return;
    }

    handle_query(std::move(callback));
}

void PostgresClient::handle_query(std::function<void(PGresult*)> callback) {
    int flush_res = PQflush(conn_);
    if (flush_res == 1) {
        // needs more writing
        auto self = shared_from_this();
        proactor_->async_wait_write(PQsocket(conn_), [self, cb = std::move(callback)]() {
            self->handle_query(cb);
        });
        return;
    } else if (flush_res == -1) {
        callback(nullptr);
        return;
    }

    // Flush done, now wait for read
    if (PQconsumeInput(conn_) == 0) {
        callback(nullptr);
        return;
    }

    if (PQisBusy(conn_)) {
        auto self = shared_from_this();
        proactor_->async_wait_read(PQsocket(conn_), [self, cb = std::move(callback)]() {
            self->handle_query(cb);
        });
        return;
    }

    // Not busy, can get result
    PGresult* res = PQgetResult(conn_);
    // Read all remaining results until null
    while (PGresult* next = PQgetResult(conn_)) {
        PQclear(res);
        res = next;
    }

    callback(res);
}

} // namespace database
