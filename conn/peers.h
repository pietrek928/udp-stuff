#pragma once

#include "../utils/common.h"
#include "../crypto/ssl.h"
#include "../net/socket.h"
#include "../net/tcpv4.h"

#include "proto.h"
#include "handler.h"

#include <cstddef>
#include <unordered_map>

typedef enum {
    TCPv4SSL,
    UDPv4,
    TCPv6SSL,
    UDPv6,
} PeerConnectionType;

typedef union {
    struct {
        in_addr_t ip;
        in_port_t port;
    } ipv4;
    // TODO: ipv6
} PeerAddressData;

class PeerConnection {
    public:

    SocketGuard socket;
    PeerConnectionType conn_type;
    PeerAddressData src_address, dst_address;

    SSL_ptr ssl;

    PeerId512_t peer_id;
    std::unordered_map<uint32_t, PeerChannelHandler> open_channel_handlers;

    PeerConnection(SocketGuard &&socket, SSL_ptr &&ssl, PeerId512_t peer_id)
        : socket(std::move(socket)), ssl(std::move(ssl)), peer_id(peer_id) {}

    bool handle_data(uint32_t type, const byte_t *data, uint32_t size);
};

/* make peer id from peer's public key */
PeerId512_t peer_id_from_sha512(const byte_t *data, size_t data_len);

PeerConnection make_peer_punch_tcpv4(SSL_CTX *ctx, PeerId512_t peer_id, const TCPv4HolePunchSettings settings);
