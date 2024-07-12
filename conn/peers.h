#pragma once

#include "../utils/common.h"
#include "../crypto/ssl.h"
#include "../net/socket.h"
#include "proto.h"
#include "handler.h"

#include <unordered_map>

class PeerConnection {
    public:

    SocketGuard socket;
    SSL_ptr ssl;

    PeerId_t peer_id;
    std::unordered_map<uint32_t, PeerChannelHandler> open_channel_handlers;
};
