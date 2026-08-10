#pragma once
#include <string>
#include <memory>
#include <functional>
#include <libpq-fe.h>
#include "../network/Proactor.hpp"

namespace database {

class PostgresClient : public std::enable_shared_from_this<PostgresClient> {
public:
    PostgresClient(std::shared_ptr<network::Proactor> proactor, const std::string& conninfo);
    ~PostgresClient();

    // Connect asynchronously
    void connect(std::function<void(bool success)> callback);

    // Execute query asynchronously
    void query(const std::string& sql, std::function<void(PGresult* res)> callback);

private:
    void handle_connect(std::function<void(bool)> callback);
    void handle_query(std::function<void(PGresult*)> callback);

    std::shared_ptr<network::Proactor> proactor_;
    std::string conninfo_;
    PGconn* conn_{nullptr};
    bool connected_{false};
};

} // namespace database
