#include "WebSocketConnection.hpp"
#include "../server/Connection.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>

namespace http {
namespace websocket {

WebSocketConnection::WebSocketConnection(server::Connection& underlying_connection)
    : connection_(underlying_connection) {}

void WebSocketConnection::on_message(std::function<void(const std::string&)> handler) {
    message_handler_ = std::move(handler);
}

void WebSocketConnection::on_close(std::function<void()> handler) {
    close_handler_ = std::move(handler);
}

void WebSocketConnection::send(const std::string& message) {
    if (is_closed_) return;

    std::vector<char> frame;
    // FIN bit set, OPCODE = 1 (text)
    frame.push_back(static_cast<char>(0x81));

    size_t len = message.length();
    // Server does not mask payloads
    if (len <= 125) {
        frame.push_back(static_cast<char>(len));
    } else if (len <= 65535) {
        frame.push_back(static_cast<char>(126));
        uint16_t ext_len = htons(static_cast<uint16_t>(len));
        char* ext_len_ptr = reinterpret_cast<char*>(&ext_len);
        frame.push_back(ext_len_ptr[0]);
        frame.push_back(ext_len_ptr[1]);
    } else {
        frame.push_back(static_cast<char>(127));
        // Use 64-bit length (network byte order)
        uint64_t ext_len = len; // Note: Assuming little-endian host, needs proper htobe64 in production
        // Extremely simple big-endian conversion
        for (int i = 7; i >= 0; --i) {
            frame.push_back(static_cast<char>((ext_len >> (i * 8)) & 0xFF));
        }
    }

    frame.insert(frame.end(), message.begin(), message.end());
    connection_.write_raw(frame);
}

void WebSocketConnection::close() {
    if (is_closed_) return;
    is_closed_ = true;

    // Send close frame (OPCODE 8)
    std::vector<char> frame = {static_cast<char>(0x88), 0x00};
    connection_.write_raw(frame);
    connection_.mark_for_close();
    
    if (close_handler_) {
        close_handler_();
    }
}

bool WebSocketConnection::parse_frame_header(const std::vector<char>& buffer, FrameHeader& header) {
    if (buffer.size() < 2) return false;

    uint8_t byte0 = static_cast<uint8_t>(buffer[0]);
    uint8_t byte1 = static_cast<uint8_t>(buffer[1]);

    header.fin = (byte0 & 0x80) != 0;
    header.opcode = byte0 & 0x0F;
    header.masked = (byte1 & 0x80) != 0;
    
    uint8_t initial_len = byte1 & 0x7F;
    header.header_length = 2;
    header.payload_length = initial_len;

    if (initial_len == 126) {
        if (buffer.size() < 4) return false;
        uint16_t ext_len;
        std::memcpy(&ext_len, buffer.data() + 2, 2);
        header.payload_length = ntohs(ext_len);
        header.header_length += 2;
    } else if (initial_len == 127) {
        if (buffer.size() < 10) return false;
        uint64_t ext_len;
        std::memcpy(&ext_len, buffer.data() + 2, 8);
        // Extremely simple big-endian conversion to host (assuming little endian host)
        uint64_t host_len = 0;
        for (int i = 0; i < 8; ++i) {
            host_len |= (static_cast<uint64_t>(static_cast<uint8_t>(buffer[static_cast<size_t>(2 + i)])) << ((7 - i) * 8));
        }
        header.payload_length = host_len;
        header.header_length += 8;
    }

    if (header.masked) {
        if (buffer.size() < header.header_length + 4) return false;
        std::memcpy(header.mask_key, buffer.data() + header.header_length, 4);
        header.header_length += 4;
    }

    // Do we have the full payload?
    if (buffer.size() < header.header_length + header.payload_length) return false;

    return true;
}

void WebSocketConnection::process_raw_data(std::vector<char>& buffer) {
    while (!buffer.empty()) {
        FrameHeader header;
        if (!parse_frame_header(buffer, header)) {
            // Need more data
            break;
        }

        // We have a full frame!
        if (header.opcode == 0x8) {
            // Close frame
            if (!is_closed_) {
                is_closed_ = true;
                if (close_handler_) close_handler_();
                connection_.mark_for_close();
            }
            break;
        } else if (header.opcode == 0x1 || header.opcode == 0x2) {
            // Text or Binary frame
            std::string payload;
            payload.resize(static_cast<size_t>(header.payload_length));
            
            const char* payload_data = buffer.data() + header.header_length;
            if (header.masked) {
                for (size_t i = 0; i < header.payload_length; ++i) {
                    payload[i] = static_cast<char>(static_cast<uint8_t>(payload_data[i]) ^ header.mask_key[i % 4]);
                }
            } else {
                std::memcpy(&payload[0], payload_data, header.payload_length);
            }

            if (message_handler_) {
                message_handler_(payload);
            }
        } else if (header.opcode == 0x9) {
            // Ping - Send Pong (0xA)
            std::vector<char> pong_frame = {static_cast<char>(0x8A), 0x00};
            connection_.write_raw(pong_frame);
        }
        
        // Remove processed frame from buffer
        buffer.erase(buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(header.header_length + header.payload_length));
    }
}

} // namespace websocket
} // namespace http
