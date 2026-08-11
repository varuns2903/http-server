#include "UdpSocket.hpp"
#include <stdexcept>
#include <cerrno>
#include <cstring>
#include <arpa/inet.h>

namespace network {

UdpSocket::UdpSocket() : Socket(::socket(AF_INET, SOCK_DGRAM, 0)) {
    if (!is_valid()) {
        throw std::runtime_error(std::string("Failed to create UDP socket: ") + std::to_string(network::get_last_socket_error()));
    }
}

void UdpSocket::bind(int port) {
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(static_cast<uint16_t>(port));
    
    // Allow address reuse
    int opt = 1;
#ifdef _WIN32
    setsockopt(fd(), SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(fd(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(fd(), SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
#endif

    if (::bind(fd(), (struct sockaddr*)&server_addr, sizeof(server_addr)) == network::SOCKET_ERROR_VAL) {
        throw std::runtime_error(std::string("Failed to bind UDP socket: ") + std::to_string(network::get_last_socket_error()));
    }
}

ssize_t UdpSocket::recv_from(char* buffer, size_t length, sockaddr_in& sender_addr) {
    socklen_t addr_len = sizeof(sender_addr);
    return ::recvfrom(fd(), buffer, length, 0, (struct sockaddr*)&sender_addr, &addr_len);
}

ssize_t UdpSocket::send_to(const char* buffer, size_t length, const sockaddr_in& dest_addr) {
    return ::sendto(fd(), buffer, length, 0, (const struct sockaddr*)&dest_addr, sizeof(dest_addr));
}

} // namespace network
