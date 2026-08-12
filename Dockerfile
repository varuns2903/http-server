# Stage 1: Build Environment
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libpq-dev \
    libhiredis-dev \
    zlib1g-dev \
    libnghttp2-dev \
    liburing-dev \
    && rm -rf /var/lib/apt/lists/*

# Build quictls (OpenSSL fork with QUIC support)
RUN git clone --depth 1 --branch openssl-3.1.4-quic1 https://github.com/quictls/openssl.git quictls && \
    cd quictls && \
    ./config --prefix=/usr/local/quictls --libdir=lib no-tests && \
    make -j$(nproc) && \
    make install_sw && \
    echo "/usr/local/quictls/lib" > /etc/ld.so.conf.d/quictls.conf && \
    ldconfig

ENV LD_LIBRARY_PATH="/usr/local/quictls/lib:${LD_LIBRARY_PATH}"
ENV PKG_CONFIG_PATH="/usr/local/quictls/lib/pkgconfig:${PKG_CONFIG_PATH}"
ENV OPENSSL_ROOT_DIR="/usr/local/quictls"

# Build nghttp3
RUN git clone --depth 1 --branch v1.6.0 https://github.com/ngtcp2/nghttp3.git && \
    cd nghttp3 && git submodule update --init && \
    mkdir build && cd build && \
    cmake -DCMAKE_INSTALL_PREFIX=/usr -DENABLE_LIB_ONLY=ON -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc) && make install

# Build ngtcp2 with quictls
RUN git clone --depth 1 --branch v1.25.0 https://github.com/ngtcp2/ngtcp2.git && \
    cd ngtcp2 && git submodule update --init && \
    mkdir build && cd build && \
    cmake -DCMAKE_INSTALL_PREFIX=/usr \
          -DENABLE_LIB_ONLY=ON \
          -DCMAKE_BUILD_TYPE=Release \
          -DENABLE_OPENSSL=ON \
          -DOPENSSL_ROOT_DIR=/usr/local/quictls \
          -DOPENSSL_INCLUDE_DIR=/usr/local/quictls/include \
          -DOPENSSL_CRYPTO_LIBRARY=/usr/local/quictls/lib/libcrypto.so \
          -DOPENSSL_SSL_LIBRARY=/usr/local/quictls/lib/libssl.so \
          .. && \
    make -j$(nproc) && make install

# Build Orbit Framework
WORKDIR /app
COPY . .
RUN mkdir -p build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DOPENSSL_ROOT_DIR=/usr/local/quictls .. && \
    make -j$(nproc)

# Stage 2: Runtime Environment
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Install runtime dependencies. 
# We include some -dev packages to lazily resolve dynamic libraries without pinning specific .so versions across Ubuntu releases.
RUN apt-get update && apt-get install -y \
    libpq5 \
    libpq-dev \
    libhiredis-dev \
    zlib1g \
    libnghttp2-14 \
    liburing2 \
    && rm -rf /var/lib/apt/lists/*

# Copy compiled QUIC/HTTP3 libraries from builder
COPY --from=builder /usr/local/quictls/lib /usr/local/quictls/lib
COPY --from=builder /usr/lib/libnghttp3* /usr/lib/
COPY --from=builder /usr/lib/libngtcp2* /usr/lib/

# Setup dynamic linker for quictls
RUN echo "/usr/local/quictls/lib" > /etc/ld.so.conf.d/quictls.conf && ldconfig

# Copy compiled applications
WORKDIR /app
COPY --from=builder /app/build/basic_server /app/basic_server
COPY --from=builder /app/build/rest_api /app/rest_api
COPY --from=builder /app/build/chat_server /app/chat_server

EXPOSE 8080 8443 8443/udp

CMD ["./basic_server"]
