#include "peers.h"

#include "../net/exception.h"
#include "../net/tcpv4.h"
#include "../crypto/auth.h"
#include "../crypto/ssl.h"
#include "../crypto/sgn.h"

#include <stdlib.h>

// TODO: peer locking, ssl write lock

std::unordered_map<PeerId512_t, PeerConnection> connected_peers;

bool PeerConnection::handle_data(uint32_t type, const byte_t *data, uint32_t size) {
    auto it = open_channel_handlers.find(type);
    if (it == open_channel_handlers.end()) {
        return false;
    }

    const PeerChannelHandler &channel_handler = it->second;
    if (unlikely(size > channel_handler.max_data_size)) {
        throw ProtocolError(
            std::string("Data size ") + std::to_string(size)
            + " exceeds maximum " + std::to_string(channel_handler.max_data_size)
        );
    }
    channel_handler.on_data(channel_handler.arg, data, size);

    return true;
}

// TODO: send lock
void send_data_to_peer(
    const PeerId512_t &peer_id, uint32_t type, const byte_t *data, uint32_t size
) {
    auto it = connected_peers.find(peer_id);
    if (it == connected_peers.end()) {
        throw std::runtime_error("Peer not connected");
    }

    PeerConnection &con = it->second;
    MessageHeader_t header = {
        .size = size,
        .type = type,
    };
    SSLWriteAllData(con.ssl.get(), (byte_t*)(&header), sizeof(MessageHeader_t));
    SSLWriteAllData(con.ssl.get(), data, size);
}

void peer_listen_thread(uint16_t port) {
    SocketGuard s = tcpv4_new_socket();
    tcpv4_bind_port(s, htons(port));
    tcpv4_listen(s, 4);
    while (true) {
        SocketGuard peer_socket;
        try {
            auto accept_result = tcpv4_accept(s);
            // TODO: store peer address
            peer_socket = accept_result.new_fd;
        } catch (const ConnectionError &e) {
            return;
        }
    }
}

// PeerConnection make_peer_punch_tcpv4(
//     SSL_CTX *ctx, PeerId512_t peer_id, const TCPv4HolePunchSettings settings
// ) {
//     SocketGuard s = tcpv4_hole_punch(settings);
//     SSL_ptr ssl = SSLCreate(ctx, s); // TODO: validate peer id here

//     return {
//         .socket = s.handle(),
//         .conn_type = TCPv4SSL,
//         .src_address = {
//             .ipv4 = {
//                 .ip = settings.src_addr,
//                 .port = settings.src_port,
//             }
//         },
//         .dst_address = {
//             .ipv4 = {
//                 .ip = settings.src_addr,
//                 .port = settings.src_port,
//             }
//         },
//         .ssl = std::move(ssl),
//         .peer_id = peer_id,
//     };
// }

void peer_support_thread(PeerConnection &con) {
    while (true) {
        MessageHeader_t header;
        SSLReadAllData(con.ssl.get(), (byte_t*)(&header), sizeof(MessageHeader_t));
        SSL_DATA_ptr data = malloc(header.size);
        if (data.is_null()) {
            throw std::runtime_error("Failed to allocate memory for data");
        }

        bool data_handled = global_handle_data(con.peer_id, header.type, data, header.size);
        if (!data_handled) {
            throw ProtocolError("No handler for data");
        }
    }
}
