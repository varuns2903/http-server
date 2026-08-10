#include "QuicConnectionManager.hpp"
#include <iostream>
#include <cstring>

namespace server {

QuicConnectionManager::QuicConnectionManager(network::UdpSocket& socket)
    : socket_(socket) {
}

QuicConnectionManager::~QuicConnectionManager() {
}

void QuicConnectionManager::on_packet_received(const uint8_t* data, size_t datalen, const sockaddr_in& sender_addr) {
    if (datalen == 0) return;
    (void)sender_addr;

    ngtcp2_version_cid ver_cid;
    
    // Decode QUIC header to extract the Destination Connection ID (DCID)
    int rv = ngtcp2_pkt_decode_version_cid(&ver_cid, data, datalen, 1);
    if (rv < 0) {
        std::cerr << "Failed to decode QUIC packet version and CID: " << ngtcp2_strerror(rv) << "\n";
        return;
    }

    ngtcp2_cid dcid_struct;
    ngtcp2_cid_init(&dcid_struct, ver_cid.dcid, ver_cid.dcidlen);

    auto it = connections_.find(dcid_struct);
    if (it != connections_.end()) {
        // Route to existing connection
        // it->second->process_packet(data, datalen, sender_addr);
    } else {
        // Handle new connection (Initial packet)
        if (ver_cid.version == 0) {
            // Version negotiation
            return;
        }
        
        std::cout << "Received QUIC packet for unknown connection, creating new QUIC connection instance (stub)\n";
        // Create new QuicConnection, add to map, and process packet
        // auto conn = std::make_shared<QuicConnection>(...);
        // connections_[dcid_struct] = conn;
        // conn->process_packet(data, datalen, sender_addr);
    }
}

void QuicConnectionManager::handle_timers() {
    // Iterate over connections and handle expiry
    // for (auto& [cid, conn] : connections_) {
    //     conn->handle_expiry();
    // }
}

} // namespace server
