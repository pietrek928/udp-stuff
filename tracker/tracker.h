#pragma once

#include "../crypto/auth.h"
#include "../conn/peers.h"

class TrackerPeerInfo {
    PeerId512_t peer_id;
    PeerConnectionType type;
    PeerAddressData address;
};