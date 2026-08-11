#pragma once

#include <ngtcp2/ngtcp2.h>
#include <ngtcp2/ngtcp2_crypto.h>
#if defined(USE_NGTCP2_CRYPTO_OSSL)
#include <ngtcp2/ngtcp2_crypto_ossl.h>
#define NGTCP2_CRYPTO_CTX ngtcp2_crypto_ossl_ctx
#define NGTCP2_CRYPTO_CTX_NEW ngtcp2_crypto_ossl_ctx_new
#define NGTCP2_CRYPTO_CTX_DEL ngtcp2_crypto_ossl_ctx_del
#define NGTCP2_CRYPTO_CONFIGURE_SERVER_SESSION(ssl) ngtcp2_crypto_ossl_configure_server_session(ssl)
#define NGTCP2_CRYPTO_CONFIGURE_SERVER_CONTEXT(ssl_ctx) (0)
#elif defined(USE_NGTCP2_CRYPTO_QUICTLS)
#include <ngtcp2/ngtcp2_crypto_quictls.h>
#define NGTCP2_CRYPTO_CTX ngtcp2_crypto_quictls_ctx
#define NGTCP2_CRYPTO_CTX_NEW ngtcp2_crypto_quictls_ctx_new
#define NGTCP2_CRYPTO_CTX_DEL ngtcp2_crypto_quictls_ctx_del
#define NGTCP2_CRYPTO_CONFIGURE_SERVER_SESSION(ssl) (0)
#define NGTCP2_CRYPTO_CONFIGURE_SERVER_CONTEXT(ssl_ctx) ngtcp2_crypto_quictls_configure_server_context(ssl_ctx)
#else
#error "No supported ngtcp2 crypto backend found"
#endif

#include <openssl/ssl.h>
#include <vector>
#include <memory>
#include <chrono>
#include <netinet/in.h>

namespace server {

class QuicConnectionManager;
class QuicHttp3Session;

class QuicConnection {
public:
    QuicConnection(QuicConnectionManager& manager, const ngtcp2_cid& client_dcid, const ngtcp2_cid& client_scid, const ngtcp2_cid& server_scid, const sockaddr_in& remote_addr, SSL_CTX* ssl_ctx);
    ~QuicConnection();

    void process_packet(const uint8_t* data, size_t datalen, const sockaddr_in& remote_addr);
    void handle_expiry();
    void send_pending_data();

private:
    QuicConnectionManager& manager_;
    sockaddr_in local_addr_;
    sockaddr_in remote_addr_;
    ngtcp2_conn* conn_{nullptr};
    SSL* ssl_{nullptr};
    
    NGTCP2_CRYPTO_CTX* tls_ctx_{nullptr};
    ngtcp2_crypto_conn_ref conn_ref_;
    
    std::unique_ptr<QuicHttp3Session> h3_session_;

    ngtcp2_tstamp get_timestamp() const;
    bool init_ssl(SSL_CTX* ssl_ctx);

    static int on_handshake_completed(ngtcp2_conn *conn, void *user_data);
    static void rand_cb(uint8_t *dest, size_t destlen, const ngtcp2_rand_ctx *rand_ctx);
    static int get_new_connection_id_cb(ngtcp2_conn *conn, ngtcp2_cid *cid, uint8_t *token, size_t cidlen, void *user_data);
    static int get_path_challenge_data_cb(ngtcp2_conn *conn, uint8_t *data, void *user_data);
    static int on_recv_stream_data(ngtcp2_conn *conn, uint32_t flags, int64_t stream_id, uint64_t offset, const uint8_t *data, size_t datalen, void *user_data, void *stream_user_data);
    static int on_get_new_connection_id(ngtcp2_conn *conn, ngtcp2_cid *cid, uint8_t *token, size_t cidlen, void *user_data);
};

} // namespace server
