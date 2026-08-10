#include "QuicConnection.hpp"
#include "QuicConnectionManager.hpp"
#include <iostream>
#include <stdexcept>

namespace server {

QuicConnection::QuicConnection(QuicConnectionManager& manager, const ngtcp2_cid& dcid, const ngtcp2_cid& scid, const sockaddr_in& remote_addr, SSL_CTX* ssl_ctx)
    : manager_(manager), remote_addr_(remote_addr) {
    
    if (!init_ssl(ssl_ctx)) {
        throw std::runtime_error("Failed to initialize SSL for QUIC connection");
    }

    ngtcp2_callbacks callbacks{};
    callbacks.client_initial = on_client_initial;
    callbacks.recv_crypto_data = on_recv_crypto_data;
    callbacks.handshake_completed = on_handshake_completed;
    callbacks.recv_stream_data = on_recv_stream_data;
    callbacks.get_new_connection_id = on_get_new_connection_id;
    callbacks.update_key = on_update_key;
    
    // We will hook up the OpenSSL crypto plugin callbacks here during full implementation
    // callbacks.encrypt = ngtcp2_crypto_encrypt_cb;
    // callbacks.decrypt = ngtcp2_crypto_decrypt_cb;
    // callbacks.hp_mask = ngtcp2_crypto_hp_mask_cb;

    ngtcp2_settings settings;
    ngtcp2_settings_default(&settings);
    settings.initial_ts = 0; // Set properly later

    ngtcp2_transport_params params;
    ngtcp2_transport_params_default(&params);
    params.initial_max_stream_data_bidi_local = 256 * 1024;
    params.initial_max_stream_data_bidi_remote = 256 * 1024;
    params.initial_max_stream_data_uni = 256 * 1024;
    params.initial_max_data = 1024 * 1024;
    params.initial_max_streams_bidi = 100;
    params.initial_max_streams_uni = 100;
    params.max_idle_timeout = 30 * NGTCP2_SECONDS;

    ngtcp2_path path = {
        { (sockaddr*)&remote_addr_, sizeof(remote_addr_) }, // remote
        { nullptr, 0 }, // local (to be filled properly)
        this
    };

    int rv = ngtcp2_conn_server_new(&conn_, &dcid, &scid, &path, NGTCP2_PROTO_VER_V1, &callbacks, &settings, &params, nullptr, this);
    if (rv != 0) {
        throw std::runtime_error("Failed to create ngtcp2 connection: " + std::string(ngtcp2_strerror(rv)));
    }
    
    ngtcp2_conn_set_tls_native_handle(conn_, ssl_);
}

QuicConnection::~QuicConnection() {
    if (conn_) ngtcp2_conn_del(conn_);
    if (ssl_) SSL_free(ssl_);
}

bool QuicConnection::init_ssl(SSL_CTX* ssl_ctx) {
    ssl_ = SSL_new(ssl_ctx);
    if (!ssl_) return false;
    
    SSL_set_app_data(ssl_, this);
    SSL_set_accept_state(ssl_);
    // SSL_set_quic_early_data_enabled(ssl_, 1); // OpenSSL 3.2+ only or disabled for now
    
    return true;
}

void QuicConnection::process_packet(const uint8_t* data, size_t datalen, const sockaddr_in& remote_addr) {
    (void)remote_addr;
    ngtcp2_path path = {
        { (sockaddr*)&remote_addr_, sizeof(remote_addr_) },
        { nullptr, 0 },
        this
    };
    
    ngtcp2_pkt_info pi{};
    int rv = ngtcp2_conn_read_pkt(conn_, &path, &pi, data, datalen, 0); // timestamp is 0 for now
    if (rv != 0) {
        std::cerr << "ngtcp2_conn_read_pkt failed: " << ngtcp2_strerror(rv) << "\n";
    }
}

void QuicConnection::handle_expiry() {
    ngtcp2_conn_handle_expiry(conn_, 0); // timestamp 0 for now
}

void QuicConnection::send_pending_data() {
    // Send packets logic here
}

int QuicConnection::on_client_initial(ngtcp2_conn *conn, void *user_data) {
    (void)conn;
    (void)user_data;
    return 0;
}

int QuicConnection::on_recv_crypto_data(ngtcp2_conn *conn, ngtcp2_encryption_level crypto_level, uint64_t offset, const uint8_t *data, size_t datalen, void *user_data) {
    (void)conn; (void)crypto_level; (void)offset; (void)data; (void)datalen; (void)user_data;
    return 0;
}

int QuicConnection::on_handshake_completed(ngtcp2_conn *conn, void *user_data) {
    (void)conn; (void)user_data;
    std::cout << "QUIC Handshake Completed!\n";
    return 0;
}

int QuicConnection::on_recv_stream_data(ngtcp2_conn *conn, uint32_t flags, int64_t stream_id, uint64_t offset, const uint8_t *data, size_t datalen, void *user_data, void *stream_user_data) {
    (void)conn; (void)flags; (void)stream_id; (void)offset; (void)data; (void)datalen; (void)user_data; (void)stream_user_data;
    return 0;
}

int QuicConnection::on_get_new_connection_id(ngtcp2_conn *conn, ngtcp2_cid *cid, uint8_t *token, size_t cidlen, void *user_data) {
    (void)conn; (void)cid; (void)token; (void)cidlen; (void)user_data;
    return 0;
}

int QuicConnection::on_update_key(ngtcp2_conn *conn, uint8_t *rx_secret, uint8_t *tx_secret, ngtcp2_crypto_aead_ctx *rx_aead_ctx, uint8_t *rx_iv, ngtcp2_crypto_aead_ctx *tx_aead_ctx, uint8_t *tx_iv, const uint8_t *current_rx_secret, const uint8_t *current_tx_secret, size_t secretlen, void *user_data) {
    (void)conn; (void)rx_secret; (void)tx_secret; (void)rx_aead_ctx; (void)rx_iv; (void)tx_aead_ctx; (void)tx_iv; (void)current_rx_secret; (void)current_tx_secret; (void)secretlen; (void)user_data;
    return 0;
}

} // namespace server
