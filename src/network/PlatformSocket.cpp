#include "PlatformSocket.hpp"
#include <stdexcept>
#include <string>

#ifndef _WIN32
#include <cstring>
#endif

namespace network {

void initialize_platform_networking() {
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        throw std::runtime_error("WSAStartup failed with error: " + std::to_string(result));
    }
#endif
}

void cleanup_platform_networking() {
#ifdef _WIN32
    WSACleanup();
#endif
}

void close_socket(socket_t fd) {
    if (fd == INVALID_SOCKET_FD) return;
#ifdef _WIN32
    closesocket(fd);
#else
    ::close(fd);
#endif
}

void set_non_blocking(socket_t fd) {
    if (fd == INVALID_SOCKET_FD) return;
#ifdef _WIN32
    u_long mode = 1;
    if (ioctlsocket(fd, FIONBIO, &mode) != NO_ERROR) {
        throw std::runtime_error("ioctlsocket FIONBIO failed with error: " + std::to_string(get_last_socket_error()));
    }
#else
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        throw std::runtime_error(std::string("fcntl F_GETFL failed: ") + std::strerror(errno));
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::runtime_error(std::string("fcntl F_SETFL O_NONBLOCK failed: ") + std::strerror(errno));
    }
#endif
}

} // namespace network
