#pragma once
#include "../network/Socket.hpp"
#include "../network/Proactor.hpp"
#include "../routing/Router.hpp"
#include "../concurrency/ThreadPool.hpp"
#include "TimerManager.hpp"
#include "../network/TlsContext.hpp"
#include "../http/ResponseWriter.hpp"
#include <vector>
#include <string_view>
#include <memory>
#include <mutex>

namespace http::websocket { class WebSocketConnection; }

namespace server {

class ConnectionManager; // Forward declaration

enum class ConnectionState {
    HTTP,
    WEBSOCKET
};

enum class RequestState {
    INCOMPLETE,
    COMPLETE,
    ERROR_PAYLOAD_TOO_LARGE,
    ERROR_HEADERS_TOO_LARGE
};

class Connection : public std::enable_shared_from_this<Connection>, public http::ResponseWriter {
public:
    Connection(network::Socket socket, const std::string& client_ip, network::Proactor& proactor, const routing::Router& router, ConnectionManager& manager, concurrency::ThreadPool& thread_pool, TimerManager& timer_manager, size_t max_body_size, network::TlsContext* tls_context = nullptr);
    ~Connection();

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    void start();

    void write_raw(const std::vector<char>& data);
    void mark_for_close();
    void upgrade_to_websocket(std::unique_ptr<http::websocket::WebSocketConnection> ws_conn);

    // ResponseWriter Implementation
    void set_header(const std::string& key, const std::string& value) override;
    network::Proactor& proactor() override { return proactor_; }
    concurrency::ThreadPool& thread_pool() override { return thread_pool_; }
    void send(http::HttpResponse&& response) override;
    void send_headers(http::HttpResponse& response) override;
    void write_chunk(std::string_view chunk) override;
    void end() override;

private:
    void process_request();
    void send_data(std::string_view data);
    void reset_timer();
    void send_error(http::HttpStatus status, const std::string& message);

    void trigger_read();
    void on_read_complete(ssize_t bytes_read);
    void trigger_write();
    void on_write_complete(ssize_t bytes_written);
    void on_sendfile_complete(ssize_t bytes_written);

    network::Socket socket_;
    std::string client_ip_;
    network::Proactor& proactor_;
    const routing::Router& router_;
    ConnectionManager& manager_;
    concurrency::ThreadPool& thread_pool_;
    TimerManager& timer_manager_;
    
    std::vector<char> read_buffer_;
    mutable std::mutex read_mutex_;
    std::vector<char> write_buffer_;
    mutable std::mutex write_mutex_;
    
    char async_read_buf_[16384]; // Buffer for kernel to write into asynchronously

    std::unordered_map<std::string, std::string> default_headers_; // Populated by middlewares
    std::string current_request_buffer_; // Holds the request data for string_views during async processing
    std::atomic<bool> is_reading_{false};
    std::atomic<bool> is_writing_{false};
    bool is_chunked_{false};

    RequestState check_request_state() const;
    bool should_close_{false};
    ConnectionState state_{ConnectionState::HTTP};
    uint64_t current_timer_id_{0};
    
    int file_fd_{-1};
    off_t file_size_{0};
    off_t file_offset_{0};
    
    size_t max_body_size_;
    
    SSL* ssl_{nullptr};
    BIO* rbio_{nullptr};
    BIO* wbio_{nullptr};
    bool is_tls_handshake_complete_{false};
    std::vector<char> tls_write_buffer_; // For holding ciphertext before sending
    
    std::atomic<bool> is_processing_request_{false};
    
    std::unique_ptr<http::websocket::WebSocketConnection> ws_connection_;
};

} // namespace server
