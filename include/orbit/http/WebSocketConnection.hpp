#pragma once
#include <string>
#include <functional>
#include <vector>
#include <cstdint>

#include <zlib.h>

namespace server {
    class Connection; // Forward declaration
}

namespace http {
namespace websocket {

/**
 * @brief Represents an active WebSocket connection.
 */
class WebSocketConnection {
public:
    explicit WebSocketConnection(server::Connection& underlying_connection, bool enable_deflate = false);
    ~WebSocketConnection();

    // User-facing API

    /**
     * @brief Registers a callback to be called when a message is received.
     * @param handler The callback function.
     */
    void on_message(std::function<void(const std::string&)> handler);

    /**
     * @brief Registers a callback to be called when the connection is closed.
     * @param handler The callback function.
     */
    void on_close(std::function<void()> handler);
    
    /**
     * @brief Sends a text message over the WebSocket connection.
     * @param message The message to send.
     */
    void send(const std::string& message);

    /**
     * @brief Closes the WebSocket connection gracefully.
     */
    void close();

    // Internal API called by Connection::handle_read when in WEBSOCKET state
    void process_raw_data(std::vector<char>& buffer);

private:
    server::Connection& connection_;
    std::function<void(const std::string&)> message_handler_;
    std::function<void()> close_handler_;
    
    bool is_closed_{false};

    struct FrameHeader {
        bool fin;
        uint8_t opcode;
        bool masked;
        uint64_t payload_length;
        uint8_t mask_key[4];
        size_t header_length;
    };

    bool parse_frame_header(const std::vector<char>& buffer, FrameHeader& header);
    
    bool deflate_enabled_{false};
    z_stream inflate_stream_{};
    z_stream deflate_stream_{};
    bool streams_initialized_{false};
    
    void init_streams();
    void cleanup_streams();
    std::string deflate_payload(const std::string& payload);
    std::string inflate_payload(const std::string& payload);
};

} // namespace websocket
} // namespace http
