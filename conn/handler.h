#pragma once

#include "../utils/common.h"
#include "../crypto/auth.h"
#include "exception.h"

#include <cstdint>
#include <vector>

// TODO: add some locking here ?

class GlobalChannelHandler {
    public:
    uint32_t max_data_size;
    void (*on_data)(void *arg, const PeerId512_t &src_id, const uint8_t *data, uint32_t size);
    void *arg;
    const char *name;

    GlobalChannelHandler() = default;
    inline GlobalChannelHandler(
        uint32_t max_data_size,
        void (*on_data)(void *arg, const PeerId512_t &src_id, const uint8_t *data, uint32_t size),
        void *arg,
        const char *name
    ) : max_data_size(max_data_size), on_data(on_data), arg(arg), name(name) {}
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
