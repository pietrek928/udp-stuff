#pragma once

#include "../utils/common.h"
#include "../crypto/ssl.h"
#include "../net/socket.h"
#include "proto.h"
#include "handler.h"
#include "../net/tcpv4.h"

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

    PeerId_t peer_id;
    std::unordered_map<uint32_t, PeerChannelHandler> open_channel_handlers;
};

PeerConnection make_peer_punch_tcpv4(SSL_CTX *ctx, PeerId_t peer_id, const TCPv4HolePunchSettings settings);
