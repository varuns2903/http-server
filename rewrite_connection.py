import sys

content = """#include "Connection.hpp"
#include "ConnectionManager.hpp"
#include "../http/HttpParser.hpp"
#include "../http/WebSocket.hpp"
#include "../http/WebSocketConnection.hpp"
#include "../config/Config.hpp"
#include <unistd.h>
#include <iostream>
#include <sys/socket.h>

namespace server {

Connection::Connection(network::Socket socket, network::Proactor& proactor, const routing::Router& router, ConnectionManager& manager, concurrency::ThreadPool& thread_pool, TimerManager& timer_manager, size_t max_body_size, network::TlsContext* tls_context)
    : socket_(std::move(socket)), proactor_(proactor), router_(router), manager_(manager), thread_pool_(thread_pool), timer_manager_(timer_manager), max_body_size_(max_body_size) {
    
    if (tls_context) {
        ssl_ = SSL_new(tls_context->get());
        rbio_ = BIO_new(BIO_s_mem());
        wbio_ = BIO_new(BIO_s_mem());
        SSL_set_bio(ssl_, rbio_, wbio_);
        SSL_set_accept_state(ssl_); // Set to server mode
    }
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
}

void Connection::reset_timer() {
    if (current_timer_id_ != 0) {
        timer_manager_.cancel_timer(current_timer_id_);
    }
    current_timer_id_ = timer_manager_.add_timer(socket_.fd(), std::chrono::seconds(10));
}

void Connection::start() {
    reset_timer();
    trigger_read();
}

void Connection::trigger_read() {
    bool expected = false;
    if (is_reading_.compare_exchange_strong(expected, true)) {
        auto self = shared_from_this();
        proactor_.async_read(socket_.fd(), async_read_buf_, sizeof(async_read_buf_), [self](ssize_t bytes) {
            self->is_reading_ = false;
            self->on_read_complete(bytes);
        });
    }
}

void Connection::on_read_complete(ssize_t bytes_read) {
    if (bytes_read <= 0) {
        manager_.remove_connection(socket_.fd());
        return;
    }
    
    reset_timer();
    
    if (ssl_) {
        BIO_write(rbio_, async_read_buf_, static_cast<int>(bytes_read));
        
        if (!is_tls_handshake_complete_) {
            int ret = SSL_do_handshake(ssl_);
            if (ret == 1) {
                is_tls_handshake_complete_ = true;
            } else {
                int err = SSL_get_error(ssl_, ret);
                if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                    char wbuf[4096];
                    while (true) {
                        int wbytes = BIO_read(wbio_, wbuf, sizeof(wbuf));
                        if (wbytes <= 0) break;
                        tls_write_buffer_.insert(tls_write_buffer_.end(), wbuf, wbuf + wbytes);
                    }
                    if (!tls_write_buffer_.empty()) {
                        trigger_write();
                    }
                    trigger_read(); // Continue reading handshake data
                    return;
                } else {
                    manager_.remove_connection(socket_.fd());
                    return;
                }
            }
        }
        
        if (is_tls_handshake_complete_) {
            while (true) {
                char clear_buf[8192];
                int ret = SSL_read(ssl_, clear_buf, sizeof(clear_buf));
                if (ret > 0) {
                    std::lock_guard<std::mutex> lock(read_mutex_);
                    read_buffer_.insert(read_buffer_.end(), clear_buf, clear_buf + ret);
                    
                    if (read_buffer_.size() > max_body_size_) {
                        manager_.remove_connection(socket_.fd());
                        return;
                    }
                } else {
                    break;
                }
            }
            
            char wbuf[4096];
            while (true) {
                int wbytes = BIO_read(wbio_, wbuf, sizeof(wbuf));
                if (wbytes <= 0) break;
                tls_write_buffer_.insert(tls_write_buffer_.end(), wbuf, wbuf + wbytes);
            }
        }
    } else {
        std::lock_guard<std::mutex> lock(read_mutex_);
        read_buffer_.insert(read_buffer_.end(), async_read_buf_, async_read_buf_ + bytes_read);
        
        if (read_buffer_.size() > max_body_size_) {
            manager_.remove_connection(socket_.fd());
            return;
        }
    }
    
    if (!tls_write_buffer_.empty()) {
        trigger_write();
    }
    
    if (state_ == ConnectionState::WEBSOCKET) {
        if (ws_connection_) {
            std::lock_guard<std::mutex> lock(read_mutex_);
            ws_connection_->process_raw_data(read_buffer_);
        }
        trigger_read();
        return;
    }
    
    if (!is_processing_request_ && is_request_complete()) {
        is_processing_request_ = true;
        auto self = shared_from_this();
        thread_pool_.enqueue([self]() {
            self->process_request();
        });
    } else {
        trigger_read();
    }
}

void Connection::process_request() {
    std::string current_buffer;
    {
        std::lock_guard<std::mutex> lock(read_mutex_);
        current_buffer = std::string(read_buffer_.begin(), read_buffer_.end());
    }
    std::string_view raw_request(current_buffer.data(), current_buffer.size());
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
                    
                    std::string handshake_str = res.serialize();
                    
                    auto ws_conn = std::make_unique<http::websocket::WebSocketConnection>(*this);
                    auto handler = router_.get_ws_route(req.uri);
                    
                    // Erase the HTTP request from the buffer before switching states!
                    size_t headers_end = raw_request.find("\\r\\n\\r\\n");
                    size_t consumed_bytes = headers_end + 4;
                    {
                        std::lock_guard<std::mutex> lock(read_mutex_);
                        if (consumed_bytes <= read_buffer_.size()) {
                            read_buffer_.erase(read_buffer_.begin(), read_buffer_.begin() + static_cast<std::ptrdiff_t>(consumed_bytes));
                        } else {
                            read_buffer_.clear();
                        }
                    }
                    
                    // Queue the handshake immediately
                    write_raw(std::vector<char>(handshake_str.begin(), handshake_str.end()));
                    
                    // Modify the state!
                    upgrade_to_websocket(std::move(ws_conn));
                    
                    // Call the user callback (this allows them to set up on_message handlers and send initial messages)
                    handler(*ws_connection_);
                    
                    // If the client pipelined a WebSocket frame immediately, process it!
                    {
                        std::lock_guard<std::mutex> lock(read_mutex_);
                        if (!read_buffer_.empty()) {
                            ws_connection_->process_raw_data(read_buffer_);
                        }
                    }
                    
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
        
        size_t headers_end = raw_request.find("\\r\\n\\r\\n");
        size_t consumed_bytes = headers_end + 4;
        
        auto cl_it = parsed_req->headers.find("Content-Length");
        if (cl_it != parsed_req->headers.end()) {
            consumed_bytes += std::stoull(std::string(cl_it->second));
        }
        
        {
            std::lock_guard<std::mutex> lock(read_mutex_);
            if (consumed_bytes <= read_buffer_.size()) {
                read_buffer_.erase(read_buffer_.begin(), read_buffer_.begin() + static_cast<std::ptrdiff_t>(consumed_bytes));
            } else {
                read_buffer_.clear(); 
            }
        }
        
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

        send_data(serialized_data);
        is_processing_request_ = false;
        
        // Request pipelining
        if (is_request_complete()) {
            is_processing_request_ = true;
            auto self = shared_from_this();
            thread_pool_.enqueue([self]() {
                self->process_request();
            });
        }
    } else {
        http::HttpResponse err_res;
        err_res.status_code = http::HttpStatus::BadRequest;
        err_res.set_body("400 Bad Request");
        err_res.headers["Connection"] = "close";
        should_close_ = true;
        
        {
            std::lock_guard<std::mutex> lock(read_mutex_);
            read_buffer_.clear();
        }
        std::string serialized = err_res.serialize();
        send_data(serialized);
        is_processing_request_ = false;
    }
}

void Connection::send_data(std::string_view data) {
    {
        std::lock_guard<std::mutex> lock(write_mutex_);
        write_buffer_.insert(write_buffer_.end(), data.begin(), data.end());
    }
    trigger_write();
}

void Connection::trigger_write() {
    bool expected = false;
    if (!is_writing_.compare_exchange_strong(expected, true)) {
        return; // Already writing
    }

    if (ssl_) {
        std::vector<char> chunk_to_encrypt;
        {
            std::lock_guard<std::mutex> lock(write_mutex_);
            if (!write_buffer_.empty()) {
                chunk_to_encrypt = std::move(write_buffer_);
                write_buffer_.clear();
            }
        }
        if (!chunk_to_encrypt.empty()) {
            SSL_write(ssl_, chunk_to_encrypt.data(), static_cast<int>(chunk_to_encrypt.size()));
        }
        
        char buf[4096];
        while (true) {
            int bytes = BIO_read(wbio_, buf, sizeof(buf));
            if (bytes <= 0) break;
            tls_write_buffer_.insert(tls_write_buffer_.end(), buf, buf + bytes);
        }
        
        if (file_fd_ != -1 && file_offset_ < file_size_ && tls_write_buffer_.empty()) {
            char file_buf[16384];
            size_t to_read = static_cast<size_t>(std::min(static_cast<off_t>(sizeof(file_buf)), file_size_ - file_offset_));
            ssize_t bytes_read = pread(file_fd_, file_buf, to_read, file_offset_);
            
            if (bytes_read > 0) {
                SSL_write(ssl_, file_buf, static_cast<int>(bytes_read));
                file_offset_ += bytes_read;
                
                while (true) {
                    int wbytes = BIO_read(wbio_, buf, sizeof(buf));
                    if (wbytes <= 0) break;
                    tls_write_buffer_.insert(tls_write_buffer_.end(), buf, buf + wbytes);
                }
            } else {
                is_writing_ = false;
                manager_.remove_connection(socket_.fd());
                return;
            }
        }
        
        if (file_fd_ != -1 && file_offset_ >= file_size_) {
            close(file_fd_);
            file_fd_ = -1;
        }
        
        if (!tls_write_buffer_.empty()) {
            auto self = shared_from_this();
            proactor_.async_write(socket_.fd(), tls_write_buffer_.data(), tls_write_buffer_.size(), [self](ssize_t written) {
                self->is_writing_ = false;
                self->on_write_complete(written);
            });
            return; // Will clear flag in callback
        }
    } else {
        std::lock_guard<std::mutex> lock(write_mutex_);
        if (!write_buffer_.empty()) {
            auto self = shared_from_this();
            proactor_.async_write(socket_.fd(), write_buffer_.data(), write_buffer_.size(), [self](ssize_t written) {
                self->is_writing_ = false;
                self->on_write_complete(written);
            });
            return;
        }
        
        if (file_fd_ != -1 && file_offset_ < file_size_) {
            auto self = shared_from_this();
            proactor_.async_sendfile(socket_.fd(), file_fd_, file_offset_, file_size_ - file_offset_, [self](ssize_t written) {
                self->is_writing_ = false;
                self->on_sendfile_complete(written);
            });
            return;
        }
        
        if (file_fd_ != -1 && file_offset_ >= file_size_) {
            close(file_fd_);
            file_fd_ = -1;
        }
    }
    
    is_writing_ = false;
    
    if (should_close_) {
        manager_.remove_connection(socket_.fd());
    }
}

void Connection::on_write_complete(ssize_t bytes_written) {
    if (bytes_written <= 0) {
        manager_.remove_connection(socket_.fd());
        return;
    }
    
    reset_timer();
    
    if (ssl_) {
        tls_write_buffer_.erase(tls_write_buffer_.begin(), tls_write_buffer_.begin() + bytes_written);
    } else {
        std::lock_guard<std::mutex> lock(write_mutex_);
        write_buffer_.erase(write_buffer_.begin(), write_buffer_.begin() + bytes_written);
    }
    
    trigger_write(); // Try writing again if needed
}

void Connection::on_sendfile_complete(ssize_t bytes_written) {
    if (bytes_written <= 0) {
        manager_.remove_connection(socket_.fd());
        return;
    }
    
    reset_timer();
    file_offset_ += bytes_written;
    
    trigger_write();
}

void Connection::write_raw(const std::vector<char>& data) {
    {
        std::lock_guard<std::mutex> lock(write_mutex_);
        write_buffer_.insert(write_buffer_.end(), data.begin(), data.end());
    }
    trigger_write();
}

void Connection::mark_for_close() {
    should_close_ = true;
    trigger_write();
}

void Connection::upgrade_to_websocket(std::unique_ptr<http::websocket::WebSocketConnection> ws_conn) {
    ws_connection_ = std::move(ws_conn);
    state_ = ConnectionState::WEBSOCKET;
}

bool Connection::is_request_complete() const {
    std::lock_guard<std::mutex> lock(read_mutex_);
    std::string_view buf_view(read_buffer_.data(), read_buffer_.size());
    size_t headers_end = buf_view.find("\\r\\n\\r\\n");
    
    if (headers_end == std::string_view::npos) {
        return false;
    }
    
    size_t content_length_pos = buf_view.find("Content-Length:");
    if (content_length_pos != std::string_view::npos && content_length_pos < headers_end) {
        size_t value_start = content_length_pos + 15;
        while (value_start < headers_end && (buf_view[value_start] == ' ' || buf_view[value_start] == '\t')) {
            value_start++;
        }
        
        size_t value_end = buf_view.find("\\r\\n", value_start);
        std::string_view length_str = buf_view.substr(value_start, value_end - value_start);
        
        try {
            size_t content_length = std::stoull(std::string(length_str));
            size_t total_expected = headers_end + 4 + content_length;
            return read_buffer_.size() >= total_expected;
        } catch (...) {
            return false;
        }
    }
    
    return true; 
}

} // namespace server
"""

with open('src/server/Connection.cpp', 'w') as f:
    f.write(content)
