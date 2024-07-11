#pragma once

#include "../utils/common.h"
#include "proto.h"

#include <cstdint>
#include <unordered_map>
#include <vector>


class GlobalChannelHandler {
    public:
    uint32_t max_data_size;
    void (*on_data)(void *arg, const PeerId_t &src_id, const uint8_t *data, uint32_t size);
    void (*show_description)(void *arg, const PeerId_t &src_id, std::vector<byte_t> &data);
    void (*destroy)(void *arg);
    void *arg;

    GlobalChannelHandler() = default;
    GlobalChannelHandler(
        uint32_t max_data_size,
        void (*on_data)(void *arg, const PeerId_t &src_id, const uint8_t *data, uint32_t size),
        void (*show_description)(void *arg, const PeerId_t &src_id, std::vector<byte_t> &data),
        void (*destroy)(void *arg), void *arg
    ) : max_data_size(max_data_size), on_data(on_data), show_description(show_description), destroy(destroy), arg(arg) {}

    GlobalChannelHandler(const GlobalChannelHandler &other) = delete;
    GlobalChannelHandler(GlobalChannelHandler &&other) = default;

    GlobalChannelHandler &operator=(const GlobalChannelHandler &other) = delete;
    GlobalChannelHandler &operator=(GlobalChannelHandler &&other) = default;

    ~GlobalChannelHandler() {
        if (destroy) {
            destroy(arg);
        }
    }
};

class PeerChannelHandler {
    public:
    uint32_t max_data_size;
    void (*on_data)(void *arg, const uint8_t *data, uint32_t size);
    void (*show_description)(void *arg, const PeerId_t &src_id, std::vector<byte_t> &data);
    void (*destroy)(void *arg);
    void *arg;

    PeerChannelHandler() = default;
    PeerChannelHandler(
        uint32_t max_data_size,
        void (*on_data)(void *arg, const uint8_t *data, uint32_t size),
        void (*show_description)(void *arg, const PeerId_t &src_id, std::vector<byte_t> &data),
        void (*destroy)(void *arg), void *arg
    ) : max_data_size(max_data_size), on_data(on_data), show_description(show_description), destroy(destroy), arg(arg) {}

    PeerChannelHandler(const PeerChannelHandler &other) = delete;
    PeerChannelHandler(PeerChannelHandler &&other) = default;

    PeerChannelHandler &operator=(const PeerChannelHandler &other) = delete;
    PeerChannelHandler &operator=(PeerChannelHandler &&other) = default;

    ~PeerChannelHandler() {
        if (destroy) {
            destroy(arg);
        }
    }
};

typedef struct PeerConnectionData {
    PeerId_t peer_id;
    std::unordered_map<uint32_t, PeerChannelHandler> open_channel_handlers;
} PeerConnectionData;


void register_global_handler(uint32_t type, GlobalChannelHandler &&handler);
bool global_handle_data(const PeerId_t &src_id, uint32_t type, const byte_t *data, uint32_t size);
