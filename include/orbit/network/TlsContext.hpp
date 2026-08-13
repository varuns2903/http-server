#pragma once
#include <string>
#include <stdexcept>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <orbit/config/Config.hpp>

namespace network {

class TlsContext {
public:
    TlsContext(const std::string& cert_file, const std::string& key_file, config::HttpVersion http_version);
    ~TlsContext();

    // Delete copy semantics
    TlsContext(const TlsContext&) = delete;
    TlsContext& operator=(const TlsContext&) = delete;

    SSL_CTX* get() const { return ctx_; }
    config::HttpVersion get_http_version() const { return http_version_; }

private:
    SSL_CTX* ctx_{nullptr};
    config::HttpVersion http_version_{config::HttpVersion::Http1_1};
};

class ClientTlsContext {
public:
    ClientTlsContext();
    ~ClientTlsContext();

    ClientTlsContext(const ClientTlsContext&) = delete;
    ClientTlsContext& operator=(const ClientTlsContext&) = delete;

    SSL_CTX* get() const { return ctx_; }

private:
    SSL_CTX* ctx_{nullptr};
};

} // namespace network
