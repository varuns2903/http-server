#pragma once

#include <nghttp3/nghttp3.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"

namespace server {

class QuicConnection;

struct Http3Stream {
    int64_t stream_id;
    http::HttpRequest request;
    std::string body_buffer;
    std::vector<std::string> header_storage;
    bool headers_complete{false};
    std::string response_body; // To store response payload
    
    Http3Stream(int64_t id) : stream_id(id) {}
};

class QuicHttp3Session {
public:
    explicit QuicHttp3Session(QuicConnection& quic_conn);
    ~QuicHttp3Session();

    bool init();
    
    // Called when QUIC stream data is received
    nghttp3_conn* get_conn() const { return httpconn_; }
    int process_stream_data(int64_t stream_id, const uint8_t* data, size_t datalen, bool fin);
    
    // Called to serialize HTTP/3 frames and feed them to ngtcp2
    int write_streams();

private:
    QuicConnection& quic_conn_;
    nghttp3_conn* httpconn_{nullptr};
    std::unordered_map<int64_t, std::shared_ptr<Http3Stream>> streams_;

    // nghttp3 callbacks
    static int on_acked_stream_data(nghttp3_conn *conn, int64_t stream_id, uint64_t datalen, void *conn_user_data, void *stream_user_data);
    static int on_stream_close(nghttp3_conn *conn, int64_t stream_id, uint64_t app_error_code, void *conn_user_data, void *stream_user_data);
    static int on_recv_data(nghttp3_conn *conn, int64_t stream_id, const uint8_t *data, size_t datalen, void *conn_user_data, void *stream_user_data);
    static int on_deferred_consume(nghttp3_conn *conn, int64_t stream_id, size_t consumed, void *conn_user_data, void *stream_user_data);
    static int on_begin_headers(nghttp3_conn *conn, int64_t stream_id, void *conn_user_data, void *stream_user_data);
    static int on_recv_header(nghttp3_conn *conn, int64_t stream_id, int32_t token, nghttp3_rcbuf *name, nghttp3_rcbuf *value, uint8_t flags, void *conn_user_data, void *stream_user_data);
    static int on_end_headers(nghttp3_conn *conn, int64_t stream_id, int fin, void *conn_user_data, void *stream_user_data);
    
    std::shared_ptr<Http3Stream> get_or_create_stream(int64_t stream_id);
    void handle_request(std::shared_ptr<Http3Stream> stream);
};

} // namespace server
