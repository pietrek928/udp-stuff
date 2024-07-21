#include "peers.h"

#include "../net/exception.h"
#include "../net/tcpv4.h"
#include "../crypto/auth.h"


std::unordered_map<PeerId512_t, PeerConnection> connected_peers;

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
