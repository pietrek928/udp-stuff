#pragma once

#include "../utils/common.h"
#include "proto.h"

#include <vector>
#include <map>


typedef struct GlobalChannelHandler {
    uint32_t max_data_size;
    void (*on_data)(int arg, const PeerId_t &src_id, const uint8_t *data, uint32_t size);
    void (*show_description)(int arg, const PeerId_t &src_id, std::vector<byte_t> &data);
    void (*unregister)(int arg);
    int arg;
} GlobalChannelHandler;

typedef struct PeerChannelHandler {
    uint32_t max_data_size;
    void (*on_data)(int arg, const uint8_t *data, uint32_t size);
    void (*show_description)(int arg, const PeerId_t &src_id, std::vector<byte_t> &data);
    void (*unregister)(int arg);
    int arg;
} PeerChannelHandler;

typedef struct PerrConnectionData {
    PeerId_t peer_id;
    std::map<int, PeerChannelHandler> open_channel_handlers;
} PerrConnectionData;
