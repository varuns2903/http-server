#pragma once
#include <string>
#include <stdexcept>
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace network {

class TlsContext {
public:
    TlsContext(const std::string& cert_file, const std::string& key_file);
    ~TlsContext();

    // Delete copy semantics
    TlsContext(const TlsContext&) = delete;
    TlsContext& operator=(const TlsContext&) = delete;

    SSL_CTX* get() const { return ctx_; }

private:
    SSL_CTX* ctx_{nullptr};
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
