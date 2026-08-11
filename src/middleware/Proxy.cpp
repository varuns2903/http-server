#include "Proxy.hpp"
#include "../network/Proactor.hpp"
#include "../utils/Logger.hpp"
#include "../network/PlatformSocket.hpp"


#ifndef _WIN32
#include <unistd.h>
#endif
#include <fcntl.h>


#include <memory>
#include <mutex>
#include "../network/ConnectionPool.hpp"
#include "../concurrency/ThreadPool.hpp"
#include "../network/TlsContext.hpp"

namespace middleware {

// Global client TLS context for the proxy
static std::unique_ptr<network::ClientTlsContext> g_proxy_tls;
static std::once_flag g_proxy_tls_flag;

class ProxyRequest : public std::enable_shared_from_this<ProxyRequest> {
public:
    ProxyRequest(network::Proactor& proactor, std::shared_ptr<http::ResponseWriter> writer,
                 const std::string& host, int port, const http::HttpRequest& original_req, bool is_https, const std::string& strip_prefix)
        : proactor_(proactor), writer_(std::move(writer)), host_(host), port_(port), original_req_(original_req), is_https_(is_https), strip_prefix_(strip_prefix) {
        
        if (is_https_) {
            std::call_once(g_proxy_tls_flag, []() {
                g_proxy_tls = std::make_unique<network::ClientTlsContext>();
            });
            
            ssl_ = SSL_new(g_proxy_tls->get());
            SSL_set_tlsext_host_name(ssl_, host_.c_str());
            
            rbio_ = BIO_new(BIO_s_mem());
            wbio_ = BIO_new(BIO_s_mem());
            SSL_set_bio(ssl_, rbio_, wbio_);
        }
    }

    ~ProxyRequest() {
        if (ssl_) SSL_free(ssl_);
        if (fd_ != -1) {
            proactor_.remove(fd_);
            network::close_socket(fd_);
        }
    }

    void start() {
        auto [fd, ssl_ptr] = network::ConnectionPool::get_instance().acquire(host_, port_);
        if (fd != -1) {
            fd_ = fd;
            if (is_https_) {
                ssl_ = static_cast<SSL*>(ssl_ptr);
            }
            is_reused_ = true;
            on_connected();
            return;
        }

        // 1. Resolve host asynchronously on the thread pool
        auto self = shared_from_this();
        writer_->thread_pool().enqueue([self]() {
            struct hostent* he = gethostbyname(self->host_.c_str());
            if (!he) {
                self->fail("DNS resolution failed");
                return;
            }

            struct sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(static_cast<uint16_t>(self->port_));
            addr.sin_addr = *(struct in_addr*)he->h_addr_list[0];

            // 2. Create socket
            self->fd_ = socket(AF_INET, SOCK_STREAM, 0);
            if (self->fd_ < 0) {
                self->fail("Failed to create socket");
                return;
            }
            fcntl(self->fd_, F_SETFL, fcntl(self->fd_, F_GETFL, 0) | O_NONBLOCK);
#ifdef FD_CLOEXEC
            fcntl(self->fd_, F_SETFD, fcntl(self->fd_, F_GETFD, 0) | FD_CLOEXEC);
#endif

            // 3. Connect asynchronously (thread-safe on Proactor)
            self->proactor_.async_connect(self->fd_, addr, [self](int status) {
                if (status != 0) {
                    self->fail("Connection refused");
                    return;
                }
                self->on_connected();
            });
        });
    }

private:
    void fail(const std::string& message) {
        LOG_ERROR("Proxy error: " << message);
        http::HttpResponse res;
        res.status(http::HttpStatus::InternalServerError).send("502 Bad Gateway: " + message);
        writer_->send(std::move(res));
    }

    void on_connected() {
        if (is_https_) {
            SSL_set_connect_state(ssl_);
            do_handshake();
            return;
        }
        build_and_send_request();
    }
    
    void do_handshake() {
        int ret = SSL_do_handshake(ssl_);
        if (ret == 1) {
            is_handshake_complete_ = true;
            build_and_send_request();
            return;
        }
        
        int err = SSL_get_error(ssl_, ret);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            flush_tls_writes([this]() {
                if (!is_handshake_complete_) {
                    read_loop();
                }
            });
        } else {
            fail("TLS handshake failed");
        }
    }
    
    void flush_tls_writes(std::function<void()> on_flushed) {
        if (!is_https_) return;
        
        int pending = BIO_pending(wbio_);
        if (pending > 0) {
            std::vector<char> buf(static_cast<size_t>(pending));
            int read_bytes = BIO_read(wbio_, buf.data(), pending);
            if (read_bytes > 0) {
                auto self = shared_from_this();
                proactor_.async_write(fd_, buf.data(), static_cast<size_t>(read_bytes), [self, cb = std::move(on_flushed)](ssize_t bytes) {
                    if (bytes <= 0) {
                        self->fail("Failed to write TLS handshake data");
                        return;
                    }
                    if (cb) cb();
                });
                return;
            }
        }
        if (on_flushed) on_flushed();
    }

    void build_and_send_request() {
        std::string method_str = "GET";
        switch (original_req_.method) {
            case http::HttpMethod::GET: method_str = "GET"; break;
            case http::HttpMethod::POST: method_str = "POST"; break;
            case http::HttpMethod::PUT: method_str = "PUT"; break;
            case http::HttpMethod::PATCH: method_str = "PATCH"; break;
            case http::HttpMethod::DELETE: method_str = "DELETE"; break;
            case http::HttpMethod::OPTIONS: method_str = "OPTIONS"; break;
            case http::HttpMethod::HEAD: method_str = "HEAD"; break;
            default: method_str = "GET"; break;
        }

        // Build forwarding request
        std::string target_path = original_req_.uri;
        if (!strip_prefix_.empty() && target_path.find(strip_prefix_) == 0) {
            target_path = target_path.substr(strip_prefix_.length());
            if (target_path.empty()) target_path = "/";
        }
        
        write_buf_ += method_str + " " + target_path + " HTTP/1.1\r\n";
        write_buf_ += "Host: " + host_ + ":" + std::to_string(port_) + "\r\n";
        
        // Forward headers (except Host and Connection)
        for (const auto& [k, v] : original_req_.headers) {
            if (k != "Host" && k != "Connection") {
                write_buf_ += std::string(k) + ": " + std::string(v) + "\r\n";
            }
        }
        
        // Inject Gateway Headers
        if (!original_req_.client_ip.empty()) {
            write_buf_ += "X-Forwarded-For: " + original_req_.client_ip + "\r\n";
            write_buf_ += "X-Real-IP: " + original_req_.client_ip + "\r\n";
        }
        write_buf_ += "X-Forwarded-Proto: " + std::string(is_https_ ? "https" : "http") + "\r\n";
        auto host_it = original_req_.headers.find("Host");
        if (host_it != original_req_.headers.end()) {
            write_buf_ += "X-Forwarded-Host: " + std::string(host_it->second) + "\r\n";
        }
        
        // We inject our own keep-alive
        write_buf_ += "Connection: keep-alive\r\n\r\n";
        
        if (!original_req_.body.empty()) {
            write_buf_ += std::string(original_req_.body);
        }

        if (is_https_) {
            SSL_write(ssl_, write_buf_.data(), static_cast<int>(write_buf_.size()));
            write_buf_.clear();
            flush_tls_writes([this]() {
                read_loop();
            });
            return;
        }

        auto self = shared_from_this();
        proactor_.async_write(fd_, write_buf_.data(), write_buf_.size(), [self](ssize_t bytes) {
            if (bytes <= 0) {
                if (self->is_reused_) {
                    self->proactor_.remove(self->fd_);
                    network::close_socket(self->fd_);
                    self->fd_ = -1;
                    self->is_reused_ = false;
                    self->start();
                    return;
                }
                self->fail("Failed to send request to upstream");
                return;
            }
            self->read_loop();
        });
    }

    void read_loop() {
        auto self = shared_from_this();
        proactor_.async_read(fd_, read_buf_, sizeof(read_buf_), [self](ssize_t bytes) {
            if (bytes > 0) {
                if (self->is_https_) {
                    BIO_write(self->rbio_, self->read_buf_, static_cast<int>(bytes));
                    if (!self->is_handshake_complete_) {
                        self->do_handshake();
                        return;
                    }
                    self->process_tls_read();
                } else {
                    self->process_cleartext(self->read_buf_, static_cast<size_t>(bytes));
                }
            } else {
                if (self->is_websocket_) {
                    return; // Upstream closed the websocket
                }
                // Done reading!
                if (!self->headers_sent_) {
                    self->send_accumulated_response(); // Fallback if it closed before headers finished
                } else {
                    self->writer_->end();
                }
            }
        });
    }
    
    void process_tls_read() {
        char buf[16384];
        while (true) {
            int bytes = SSL_read(ssl_, buf, sizeof(buf));
            if (bytes > 0) {
                process_cleartext(buf, static_cast<size_t>(bytes));
            } else {
                break;
            }
        }
        // If SSL_read generated a write, flush it (e.g. renegotiation or alerts)
        flush_tls_writes(nullptr);
    }

    void process_cleartext(const char* data, size_t len) {
        if (!headers_sent_) {
            response_accumulator_.append(data, len);
            size_t header_end = response_accumulator_.find("\r\n\r\n");
            if (header_end != std::string::npos) {
                parse_and_send_headers(header_end);
                headers_sent_ = true;
                
                size_t trailing_bytes = 0;
                if (header_end + 4 < response_accumulator_.size()) {
                    std::string trailing = response_accumulator_.substr(header_end + 4);
                    writer_->write_chunk(trailing);
                    trailing_bytes = trailing.size();
                }
                response_accumulator_.clear(); // Free memory
                
                body_bytes_read_ += trailing_bytes;
                check_completion();
            } else {
                read_loop();
            }
        } else {
            std::string_view chunk(data, len);
            if (is_websocket_) {
                writer_->write_chunk(chunk); // write_chunk still works to send raw data in our implementation
            } else {
                writer_->write_chunk(chunk);
                body_bytes_read_ += len;
                check_completion();
            }
        }
    }

    void check_completion() {
        if (is_keep_alive_eligible_ && body_bytes_read_ >= content_length_) {
            writer_->end();
            network::ConnectionPool::get_instance().release(host_, port_, fd_, ssl_);
            proactor_.remove(fd_);
            fd_ = -1; // Prevent destructor from closing
            ssl_ = nullptr; // Prevent destructor from freeing
            return;
        }
        // If chunked, we should theoretically parse chunks here, but for now we'll 
        // rely on EOF (which means no keep-alive for chunked responses right now).
        read_loop();
    }

    void parse_and_send_headers(size_t header_end) {
        http::HttpResponse res;
        std::string headers_part = response_accumulator_.substr(0, header_end);
        
        size_t first_space = headers_part.find(' ');
        if (first_space != std::string::npos) {
            size_t second_space = headers_part.find(' ', first_space + 1);
            if (second_space != std::string::npos) {
                std::string code_str = headers_part.substr(first_space + 1, second_space - first_space - 1);
                res.status_code = static_cast<http::HttpStatus>(std::stoi(code_str));
            }
        }
        
        size_t pos = headers_part.find("\r\n");
        while (pos != std::string::npos) {
            size_t next = headers_part.find("\r\n", pos + 2);
            std::string line = (next == std::string::npos) 
                ? headers_part.substr(pos + 2) 
                : headers_part.substr(pos + 2, next - pos - 2);
            
            size_t colon = line.find(": ");
            if (colon != std::string::npos) {
                std::string key = line.substr(0, colon);
                std::string val = line.substr(colon + 2);
                
                if (key == "Content-Length") {
                    content_length_ = std::stoull(val);
                    is_keep_alive_eligible_ = true;
                } else if (key == "Connection" && val == "close") {
                    is_keep_alive_eligible_ = false;
                }
                
                if (key != "Transfer-Encoding" && key != "Connection") {
                    res.headers[key] = val;
                }
            }
            pos = next;
        }
        
        
        writer_->send_headers(res);

        if (res.status_code == http::HttpStatus::SwitchingProtocols) {
            is_websocket_ = true;
            auto self = shared_from_this();
            writer_->upgrade_to_raw_stream(
                [self](std::string_view data) {
                    if (self->is_https_) {
                        SSL_write(self->ssl_, data.data(), static_cast<int>(data.size()));
                        self->flush_tls_writes(nullptr);
                    } else {
                        // We must copy data to a stable buffer for async_write
                        // This is a naive implementation for demonstration
                        std::vector<char>* buf = new std::vector<char>(data.begin(), data.end());
                        self->proactor_.async_write(self->fd_, buf->data(), buf->size(), [buf](ssize_t) {
                            delete buf;
                        });
                    }
                },
                [self]() {
                    if (self->fd_ != -1) {
                        self->proactor_.remove(self->fd_);
                        network::close_socket(self->fd_);
                        self->fd_ = -1;
                    }
                }
            );
            read_loop();
        }
    }

    void send_accumulated_response() {
        // Fallback for malformed responses
        http::HttpResponse res;
        res.body = response_accumulator_;
        res.status_code = http::HttpStatus::OK;
        writer_->send(std::move(res));
    }

    network::Proactor& proactor_;
    std::shared_ptr<http::ResponseWriter> writer_;
    std::string host_;
    int port_;
    const http::HttpRequest& original_req_;
    bool is_https_;
    std::string strip_prefix_;
    
    int fd_{-1};
    SSL* ssl_{nullptr};
    BIO* rbio_{nullptr};
    BIO* wbio_{nullptr};
    bool is_handshake_complete_{false};

    std::string write_buf_;
    char read_buf_[16384];
    
    std::string response_accumulator_;
    bool headers_sent_{false};
    bool is_reused_{false};
    bool is_keep_alive_eligible_{false};
    bool is_websocket_{false};
    size_t content_length_{0};
    size_t body_bytes_read_{0};
};

routing::Middleware proxy(ProxyOptions options) {
    return [options](http::HttpRequest& request, std::shared_ptr<http::ResponseWriter> writer) -> bool {
        bool is_https = (options.target_port == 443);
        auto proxy_req = std::make_shared<ProxyRequest>(writer->proactor(), writer, options.target_host, options.target_port, request, is_https, options.strip_prefix);
        proxy_req->start();
        return false;
    };
}

routing::Middleware proxy(const std::string& target_host, int target_port) {
    ProxyOptions opts;
    opts.target_host = target_host;
    opts.target_port = target_port;
    return proxy(opts);
}

routing::Middleware load_balancer(LoadBalancerOptions options) {
    if (options.nodes.empty()) {
        throw std::runtime_error("Load balancer requires at least one target node");
    }
    
    auto current_node = std::make_shared<std::atomic<size_t>>(0);
    
    return [options, current_node](http::HttpRequest& request, std::shared_ptr<http::ResponseWriter> writer) -> bool {
        size_t idx = current_node->fetch_add(1, std::memory_order_relaxed) % options.nodes.size();
        const auto& target = options.nodes[idx];
        
        bool is_https = (target.port == 443);
        auto proxy_req = std::make_shared<ProxyRequest>(writer->proactor(), writer, target.host, target.port, request, is_https, options.strip_prefix);
        proxy_req->start();
        return false;
    };
}

routing::Middleware load_balancer(const std::vector<TargetNode>& nodes) {
    LoadBalancerOptions opts;
    opts.nodes = nodes;
    return load_balancer(opts);
}

} // namespace middleware
