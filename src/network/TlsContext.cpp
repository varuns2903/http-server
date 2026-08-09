#include "TlsContext.hpp"
#include "../utils/Logger.hpp"

namespace network {

TlsContext::TlsContext(const std::string& cert_file, const std::string& key_file) {
    const SSL_METHOD* method = TLS_server_method();
    ctx_ = SSL_CTX_new(method);
    if (!ctx_) {
        throw std::runtime_error("Unable to create SSL context");
    }

    SSL_CTX_set_min_proto_version(ctx_, TLS1_2_VERSION);

    if (SSL_CTX_use_certificate_chain_file(ctx_, cert_file.c_str()) <= 0) {
        ERR_print_errors_fp(stderr);
        throw std::runtime_error("Failed to load certificate file: " + cert_file);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx_, key_file.c_str(), SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        throw std::runtime_error("Failed to load private key file: " + key_file);
    }

    if (!SSL_CTX_check_private_key(ctx_)) {
        throw std::runtime_error("Private key does not match the certificate public key");
    }

    LOG_INFO("TLS Context initialized successfully with " << cert_file);
}

TlsContext::~TlsContext() {
    if (ctx_) {
        SSL_CTX_free(ctx_);
    }
}

} // namespace network
