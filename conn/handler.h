#pragma once

#include "../utils/common.h"
#include "../crypto/auth.h"

#include <cstdint>
#include <vector>


class GlobalChannelHandler {
    public:
    uint32_t max_data_size;
    void (*on_data)(void *arg, const PeerId512_t &src_id, const uint8_t *data, uint32_t size);
    void (*show_description)(void *arg, const PeerId512_t &src_id, std::vector<byte_t> &data);
    void (*destroy)(void *arg);
    void *arg;

    GlobalChannelHandler() = default;
    inline GlobalChannelHandler(
        uint32_t max_data_size,
        void (*on_data)(void *arg, const PeerId512_t &src_id, const uint8_t *data, uint32_t size),
        void (*show_description)(void *arg, const PeerId512_t &src_id, std::vector<byte_t> &data),
        void (*destroy)(void *arg), void *arg
    ) : max_data_size(max_data_size), on_data(on_data), show_description(show_description), destroy(destroy), arg(arg) {}

    GlobalChannelHandler(const GlobalChannelHandler &other) = delete;
    GlobalChannelHandler(GlobalChannelHandler &&other) = default;

    GlobalChannelHandler &operator=(const GlobalChannelHandler &other) = delete;
    GlobalChannelHandler &operator=(GlobalChannelHandler &&other) = default;

    inline ~GlobalChannelHandler() {
        if (destroy) {
            destroy(arg);
        }
    }
};

class PeerChannelHandler {
    public:
    uint32_t max_data_size;
    void (*on_data)(void *arg, const uint8_t *data, uint32_t size);
    void (*show_description)(void *arg, const PeerId512_t &src_id, std::vector<byte_t> &data);
    void (*destroy)(void *arg);
    void *arg;

    PeerChannelHandler() = default;
    inline PeerChannelHandler(
        uint32_t max_data_size,
        void (*on_data)(void *arg, const uint8_t *data, uint32_t size),
        void (*show_description)(void *arg, const PeerId512_t &src_id, std::vector<byte_t> &data),
        void (*destroy)(void *arg), void *arg
    ) : max_data_size(max_data_size), on_data(on_data), show_description(show_description), destroy(destroy), arg(arg) {}

    PeerChannelHandler(const PeerChannelHandler &other) = delete;
    PeerChannelHandler(PeerChannelHandler &&other) = default;

    PeerChannelHandler &operator=(const PeerChannelHandler &other) = delete;
    PeerChannelHandler &operator=(PeerChannelHandler &&other) = default;

    inline ~PeerChannelHandler() {
        if (destroy) {
            destroy(arg);
        }
    }
};

void register_global_handler(uint32_t type, GlobalChannelHandler &&handler);
bool global_handle_data(const PeerId512_t &src_id, uint32_t type, const byte_t *data, uint32_t size);
