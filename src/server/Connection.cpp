#include "Connection.hpp"
#include "ConnectionManager.hpp"
#include "../http/HttpParser.hpp"
#include "../http/WebSocket.hpp"
#include "../http/WebSocketConnection.hpp"
#include "../config/Config.hpp"
#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/sendfile.h>

namespace server {

Connection::Connection(network::Socket socket, network::Epoll& epoll, const routing::Router& router, ConnectionManager& manager, concurrency::ThreadPool& thread_pool, TimerManager& timer_manager, size_t max_body_size, network::TlsContext* tls_context)
    : socket_(std::move(socket)), epoll_(epoll), router_(router), manager_(manager), thread_pool_(thread_pool), timer_manager_(timer_manager), max_body_size_(max_body_size) {
    
    if (tls_context) {
        ssl_ = SSL_new(tls_context->get());
        SSL_set_fd(ssl_, socket_.fd());
        SSL_set_accept_state(ssl_); // Set to server mode
    }
    
    reset_timer(); // Start the 10-second idle timeout
}

Connection::~Connection() {
    if (current_timer_id_ != 0) {
        timer_manager_.cancel_timer(current_timer_id_);
    }
    if (ssl_) {
        SSL_free(ssl_);
    }
    if (file_fd_ != -1) {
        close(file_fd_);
    }
    // The RAII network::Socket destructor will automatically close the socket FD
}

void Connection::reset_timer() {
    if (current_timer_id_ != 0) {
        timer_manager_.cancel_timer(current_timer_id_);
    }
    // Keep-Alive timeout is 10 seconds
    current_timer_id_ = timer_manager_.add_timer(socket_.fd(), std::chrono::seconds(10));
}

void Connection::handle_read() {
    reset_timer(); // Client sent data, reset their timeout!
    
    if (ssl_ && !is_tls_handshake_complete_) {
        int ret = SSL_do_handshake(ssl_);
        if (ret == 1) {
            is_tls_handshake_complete_ = true;
        } else {
            int err = SSL_get_error(ssl_, ret);
            if (err == SSL_ERROR_WANT_READ) {
                epoll_.modify(socket_.fd(), EPOLLIN | EPOLLONESHOT);
                return;
            } else if (err == SSL_ERROR_WANT_WRITE) {
                epoll_.modify(socket_.fd(), EPOLLIN | EPOLLOUT | EPOLLONESHOT);
                return;
            } else {
                std::cerr << "TLS handshake failed on FD " << socket_.fd() << std::endl;
                manager_.remove_connection(socket_.fd());
                return;
            }
        }
    }
    
    char buffer[4096];
    
    // Read from the non-blocking socket until EAGAIN
    while (true) {
        ssize_t bytes_read;
        if (ssl_) {
            bytes_read = SSL_read(ssl_, buffer, sizeof(buffer));
        } else {
            bytes_read = recv(socket_.fd(), buffer, sizeof(buffer), 0);
        }

        if (bytes_read <= 0) {
            if (ssl_) {
                int err = SSL_get_error(ssl_, static_cast<int>(bytes_read));
                if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                    break;
                }
            } else {
                if (bytes_read == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    break; // No more data right now, exit the read loop
                }
            }
            manager_.remove_connection(socket_.fd());
            return;
        } else {
            // Append newly read bytes to our persistent buffer
            read_buffer_.insert(read_buffer_.end(), buffer, buffer + bytes_read);
            
            // Limit check to prevent memory exhaustion (DDoS protection)
            if (read_buffer_.size() > max_body_size_) {
                std::cerr << "WARN: Connection " << socket_.fd() << " exceeded max_body_size (" << max_body_size_ << "). Dropping." << std::endl;
                manager_.remove_connection(socket_.fd());
                return;
            }
        }
    }

    if (state_ == ConnectionState::WEBSOCKET) {
        if (ws_connection_) {
            ws_connection_->process_raw_data(read_buffer_);
        }
        // Need to re-arm EPOLLONESHOT so we can read next frame
        epoll_.modify(socket_.fd(), EPOLLIN | EPOLLONESHOT);
        return;
    }

    // Now, check if we have a full HTTP request buffered
    if (!is_request_complete()) {
        // We hit EAGAIN but don't have a full request. We must re-arm EPOLLONESHOT!
        epoll_.modify(socket_.fd(), EPOLLIN | EPOLLONESHOT);
        return; 
    }

    // We have a full request! Offload parsing and routing to a worker thread!
    thread_pool_.enqueue([this]() {
        this->process_request();
    });
}

void Connection::process_request() {
    std::string_view raw_request(read_buffer_.data(), read_buffer_.size());
    auto parsed_req = http::HttpParser::parse(raw_request);
    
    if (parsed_req) {
        http::HttpRequest& req = *parsed_req;
        
        // WebSocket Upgrade Interception
        auto upgrade_it = req.headers.find("Upgrade");
        if (upgrade_it != req.headers.end() && upgrade_it->second == "websocket") {
            if (router_.has_ws_route(req.uri)) {
                auto key_it = req.headers.find("Sec-WebSocket-Key");
                if (key_it != req.headers.end()) {
                    std::string accept_key = http::websocket::Handshake::generate_accept_key(std::string(key_it->second));
                    
                    http::HttpResponse res;
                    res.status_code = http::HttpStatus::SwitchingProtocols;
                    res.headers["Upgrade"] = "websocket";
                    res.headers["Connection"] = "Upgrade";
                    res.headers["Sec-WebSocket-Accept"] = accept_key;
                    
                    send_data(res.serialize());
                    
                    auto ws_conn = std::make_unique<http::websocket::WebSocketConnection>(*this);
                    auto handler = router_.get_ws_route(req.uri);
                    
                    // Call the user callback synchronously (it configures the event handlers)
                    handler(*ws_conn);
                    
                    // Modify the state!
                    upgrade_to_websocket(std::move(ws_conn));
                    return; // Bypass standard HTTP routing
                }
            }
        }

        http::HttpResponse response;
        router_.route(req, response);
        
        bool keep_alive = true;
        auto it = req.headers.find("Connection");
        if (it != req.headers.end() && it->second == "close") {
            keep_alive = false;
        }

        if (!keep_alive) {
            response.headers["Connection"] = "close";
            should_close_ = true; 
        } else {
            response.headers["Connection"] = "keep-alive";
        }
        
        // 1. Calculate how many bytes to consume from read_buffer_
        size_t headers_end = raw_request.find("\r\n\r\n");
        size_t consumed_bytes = headers_end + 4;
        
        auto cl_it = parsed_req->headers.find("Content-Length");
        if (cl_it != parsed_req->headers.end()) {
            consumed_bytes += std::stoull(std::string(cl_it->second));
        }
        
        // 2. Erase the request from read_buffer_ BEFORE sending data!
        // This prevents a Use-After-Free race condition if the client disconnects immediately.
        if (consumed_bytes <= read_buffer_.size()) {
            read_buffer_.erase(read_buffer_.begin(), read_buffer_.begin() + static_cast<std::ptrdiff_t>(consumed_bytes));
        } else {
            read_buffer_.clear(); 
        }
        
        // 3. Setup file state BEFORE sending headers!
        bool is_file = (response.file_fd != -1);
        std::string serialized_data;
        
        if (is_file) {
            serialized_data = response.serialize_headers();
            this->file_fd_ = response.file_fd;
            this->file_size_ = response.file_size;
            this->file_offset_ = 0;
            response.file_fd = -1; // Steal the FD
        } else {
            serialized_data = response.serialize();
        }

        // 4. FINALLY, send data! This might yield to EventLoop! Do not touch `this` after this line!
        send_data(serialized_data);
    } else {
        http::HttpResponse err_res;
        err_res.status_code = http::HttpStatus::BadRequest;
        err_res.set_body("400 Bad Request");
        err_res.headers["Connection"] = "close";
        should_close_ = true;
        
        read_buffer_.clear(); // Clear bad request before sending
        std::string serialized = err_res.serialize();
        send_data(serialized);
    }
}

void Connection::send_data(std::string_view data) {
    write_buffer_.insert(write_buffer_.end(), data.begin(), data.end());
    handle_write();
}

void Connection::handle_write() {
    reset_timer(); // Writing to client, reset their timeout!
    
    // 1. Drain the write_buffer_ (which holds headers or string bodies)
    std::vector<char> chunk_to_write;
    {
        std::lock_guard<std::mutex> lock(write_mutex_);
        if (!write_buffer_.empty()) {
            chunk_to_write = write_buffer_; // Copy to avoid holding lock during I/O
        }
    }

    if (!chunk_to_write.empty()) {
        ssize_t bytes_sent;
        if (ssl_) {
            bytes_sent = SSL_write(ssl_, chunk_to_write.data(), static_cast<int>(chunk_to_write.size()));
        } else {
            bytes_sent = send(socket_.fd(), chunk_to_write.data(), chunk_to_write.size(), 0);
        }
        
        if (bytes_sent <= 0) {
            if (ssl_) {
                int err = SSL_get_error(ssl_, static_cast<int>(bytes_sent));
                if (err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ) {
                    epoll_.modify(socket_.fd(), EPOLLIN | EPOLLOUT | EPOLLONESHOT);
                    return;
                }
            } else {
                if (bytes_sent == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    epoll_.modify(socket_.fd(), EPOLLIN | EPOLLOUT | EPOLLONESHOT);
                    return;
                }
            }
            manager_.remove_connection(socket_.fd());
            return;
        }
        
        {
            std::lock_guard<std::mutex> lock(write_mutex_);
            write_buffer_.erase(write_buffer_.begin(), write_buffer_.begin() + static_cast<std::ptrdiff_t>(bytes_sent));
            if (!write_buffer_.empty()) {
                epoll_.modify(socket_.fd(), EPOLLIN | EPOLLOUT | EPOLLONESHOT);
                return; // Still have headers to send
            }
        }
    }

    // 2. If we have a file to send, do it via zero-copy sendfile (or SSL_write for TLS)
    if (file_fd_ != -1 && file_offset_ < file_size_) {
        if (ssl_) {
            // Cannot use zero-copy sendfile with TLS. Must read into userspace and SSL_write.
            char file_buf[4096];
            size_t to_read = static_cast<size_t>(std::min(static_cast<off_t>(sizeof(file_buf)), file_size_ - file_offset_));
            ssize_t bytes_read = pread(file_fd_, file_buf, to_read, file_offset_);
            
            if (bytes_read > 0) {
                ssize_t bytes_sent = SSL_write(ssl_, file_buf, static_cast<int>(bytes_read));
                if (bytes_sent <= 0) {
                    int err = SSL_get_error(ssl_, static_cast<int>(bytes_sent));
                    if (err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ) {
                        epoll_.modify(socket_.fd(), EPOLLIN | EPOLLOUT | EPOLLONESHOT);
                        return;
                    }
                    manager_.remove_connection(socket_.fd());
                    return;
                }
                file_offset_ += bytes_sent;
            } else {
                // File read error
                manager_.remove_connection(socket_.fd());
                return;
            }
        } else {
            // file_offset_ is updated automatically by sendfile
            ssize_t sent = sendfile(socket_.fd(), file_fd_, &file_offset_, static_cast<size_t>(file_size_ - file_offset_));
            
            if (sent == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    epoll_.modify(socket_.fd(), EPOLLIN | EPOLLOUT | EPOLLONESHOT);
                    return;
                }
                manager_.remove_connection(socket_.fd());
                return;
            }
        }
        
        if (file_offset_ < file_size_) {
            // Still more file data to send
            epoll_.modify(socket_.fd(), EPOLLIN | EPOLLOUT | EPOLLONESHOT);
            return;
        }
        
        // We finished sending the entire file!
        close(file_fd_);
        file_fd_ = -1;
    }

    // After writing, if we should close the connection, do so.
    // However, if we are in WEBSOCKET state, should_close_ indicates the close frame was sent.
    if (should_close_) {
        manager_.remove_connection(socket_.fd());
        return;
    }
    
    epoll_.modify(socket_.fd(), EPOLLIN | EPOLLONESHOT);
    
    if (state_ == ConnectionState::HTTP && is_request_complete()) {
        thread_pool_.enqueue([this]() {
            this->process_request();
        });
    }
}

void Connection::write_raw(const std::vector<char>& data) {
    {
        std::lock_guard<std::mutex> lock(write_mutex_);
        write_buffer_.insert(write_buffer_.end(), data.begin(), data.end());
    }
    epoll_.modify(socket_.fd(), EPOLLIN | EPOLLOUT | EPOLLONESHOT);
}

void Connection::mark_for_close() {
    should_close_ = true;
    epoll_.modify(socket_.fd(), EPOLLIN | EPOLLOUT | EPOLLONESHOT);
}

void Connection::upgrade_to_websocket(std::unique_ptr<http::websocket::WebSocketConnection> ws_conn) {
    ws_connection_ = std::move(ws_conn);
    state_ = ConnectionState::WEBSOCKET;
}

bool Connection::is_request_complete() const {
    std::string_view buf_view(read_buffer_.data(), read_buffer_.size());
    size_t headers_end = buf_view.find("\r\n\r\n");
    
    if (headers_end == std::string_view::npos) {
        return false; // Headers not fully received yet
    }
    
    // We have all headers. Check if there's a Content-Length indicating a body.
    size_t content_length_pos = buf_view.find("Content-Length:");
    if (content_length_pos != std::string_view::npos && content_length_pos < headers_end) {
        size_t value_start = content_length_pos + 15;
        while (value_start < headers_end && (buf_view[value_start] == ' ' || buf_view[value_start] == '\t')) {
            value_start++;
        }
        
        size_t value_end = buf_view.find("\r\n", value_start);
        std::string_view length_str = buf_view.substr(value_start, value_end - value_start);
        
        try {
            size_t content_length = std::stoull(std::string(length_str));
            size_t total_expected = headers_end + 4 + content_length;
            return read_buffer_.size() >= total_expected;
        } catch (...) {
            return false; // Malformed header
        }
    }
    
    // No Content-Length? Then we assume the request ends after \r\n\r\n
    return true; 
}

} // namespace server
