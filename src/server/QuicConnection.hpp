#pragma once

#include <ngtcp2/ngtcp2.h>
#include <ngtcp2/ngtcp2_crypto.h>
#include <ngtcp2/ngtcp2_crypto_ossl.h>
#include <openssl/ssl.h>
#include <vector>
#include <memory>
#include <netinet/in.h>

namespace server {

class QuicConnectionManager;

class QuicConnection {
public:
    QuicConnection(QuicConnectionManager& manager, const ngtcp2_cid& dcid, const ngtcp2_cid& scid, const sockaddr_in& remote_addr, SSL_CTX* ssl_ctx);
    ~QuicConnection();

    void process_packet(const uint8_t* data, size_t datalen, const sockaddr_in& remote_addr);
    void handle_expiry();
    void send_pending_data();

private:
    QuicConnectionManager& manager_;
    ngtcp2_conn* conn_{nullptr};
    SSL* ssl_{nullptr};
    sockaddr_in remote_addr_;
    
    bool init_ssl(SSL_CTX* ssl_ctx);

    // Callbacks for ngtcp2
    static int on_client_initial(ngtcp2_conn *conn, void *user_data);
    static int on_recv_crypto_data(ngtcp2_conn *conn, ngtcp2_encryption_level crypto_level, uint64_t offset, const uint8_t *data, size_t datalen, void *user_data);
    static int on_handshake_completed(ngtcp2_conn *conn, void *user_data);
    static int on_recv_stream_data(ngtcp2_conn *conn, uint32_t flags, int64_t stream_id, uint64_t offset, const uint8_t *data, size_t datalen, void *user_data, void *stream_user_data);
    static int on_get_new_connection_id(ngtcp2_conn *conn, ngtcp2_cid *cid, uint8_t *token, size_t cidlen, void *user_data);
    static int on_update_key(ngtcp2_conn *conn, uint8_t *rx_secret, uint8_t *tx_secret, ngtcp2_crypto_aead_ctx *rx_aead_ctx, uint8_t *rx_iv, ngtcp2_crypto_aead_ctx *tx_aead_ctx, uint8_t *tx_iv, const uint8_t *current_rx_secret, const uint8_t *current_tx_secret, size_t secretlen, void *user_data);
};

} // namespace server
