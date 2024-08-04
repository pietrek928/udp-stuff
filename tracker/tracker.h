#pragma once

#include "../crypto/auth.h"
#include "../conn/peers.h"

typedef struct {
    PeerId512_t peer_id;
    PeerConnectionType type;
    PeerAddressData address;
    std::string description;
} TrackerPeerInfo;

typedef struct {
    PeerId512_t peer_id;
    PeerConnectionType address_type;
    PeerAddressData start_address;
    PeerAddressData end_address;
    std::string description;
} ConnForwardInfo;
