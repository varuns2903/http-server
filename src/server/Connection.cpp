#include "Connection.hpp"
#include "ConnectionManager.hpp"
#include "../http/HttpParser.hpp"
#include <iostream>
#include <sys/socket.h>
#include <sys/sendfile.h>

namespace server {

Connection::Connection(network::Socket socket, network::Epoll& epoll, const routing::Router& router, ConnectionManager& manager, concurrency::ThreadPool& thread_pool, TimerManager& timer_manager)
    : socket_(std::move(socket)), epoll_(epoll), router_(router), manager_(manager), thread_pool_(thread_pool), timer_manager_(timer_manager) {
    reset_timer(); // Start the 10-second idle timeout
}

Connection::~Connection() {
    if (current_timer_id_ != 0) {
        timer_manager_.cancel_timer(current_timer_id_);
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
    
    char buffer[4096];
    
    // Read from the non-blocking socket until EAGAIN
    while (true) {
        ssize_t bytes_read = recv(socket_.fd(), buffer, sizeof(buffer), 0);

        if (bytes_read == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break; // No more data right now, exit the read loop
            }
            manager_.remove_connection(socket_.fd());
            return;
        } else if (bytes_read == 0) {
            manager_.remove_connection(socket_.fd());
            return;
        } else {
            // Append newly read bytes to our persistent buffer
            read_buffer_.insert(read_buffer_.end(), buffer, buffer + bytes_read);
        }
    }

    // Now, check if we have a full request buffered
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
        http::HttpResponse response = router_.route(*parsed_req);
        
        bool keep_alive = true;
        auto it = parsed_req->headers.find("Connection");
        if (it != parsed_req->headers.end() && it->second == "close") {
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
    if (!write_buffer_.empty()) {
        ssize_t bytes_sent = send(socket_.fd(), write_buffer_.data(), write_buffer_.size(), 0);
        if (bytes_sent == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                epoll_.modify(socket_.fd(), EPOLLIN | EPOLLOUT | EPOLLONESHOT);
                return;
            }
            manager_.remove_connection(socket_.fd());
            return;
        }
        write_buffer_.erase(write_buffer_.begin(), write_buffer_.begin() + static_cast<std::ptrdiff_t>(bytes_sent));
        
        if (!write_buffer_.empty()) {
            epoll_.modify(socket_.fd(), EPOLLIN | EPOLLOUT | EPOLLONESHOT);
            return; // Still have headers to send
        }
    }

    // 2. If we have a file to send, do it via zero-copy sendfile
    if (file_fd_ != -1 && file_offset_ < file_size_) {
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
        
        if (file_offset_ < file_size_) {
            // Still more file data to send
            epoll_.modify(socket_.fd(), EPOLLIN | EPOLLOUT | EPOLLONESHOT);
            return;
        }
        
        // We finished sending the entire file!
        close(file_fd_);
        file_fd_ = -1;
    }

    // 3. We finished writing everything!
    if (should_close_) {
        manager_.remove_connection(socket_.fd());
    } else {
        // Revert to only watching for new incoming requests
        epoll_.modify(socket_.fd(), EPOLLIN | EPOLLONESHOT);
        
        // Wait! If there is STILL data in the read_buffer_ from pipelining, 
        // we should process it immediately instead of waiting for epoll!
        if (is_request_complete()) {
            thread_pool_.enqueue([this]() {
                this->process_request();
            });
        }
    }
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
