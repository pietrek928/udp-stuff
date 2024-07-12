#include "peers.h"

#include "../net/exception.h"
#include "../net/tcpv4.h"

std::unordered_map<PeerId_t, PeerConnection> connected_peers;

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

void make_peer_connection(SSL_CTX *ctx, const TCPv4HolePunchSettings settings) {
    SocketGuard s = tcpv4_hole_punch(settings);
    SSL_ptr ssl = SSLCreate(ctx, s);
}
