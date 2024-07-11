#include "handler.h"

#include <cstdint>
#include <stdexcept>
#include <unordered_map>


std::unordered_map<uint32_t, GlobalChannelHandler> global_channel_handlers;

void register_global_handler(uint32_t type, GlobalChannelHandler &&handler) {
    if (global_channel_handlers.find(type) != global_channel_handlers.end()) {
        throw std::runtime_error("Handler already registered");
    }
    global_channel_handlers[type] = handler;
}

bool global_handle_data(const PeerId_t &src_id, uint32_t type, const byte_t *data, uint32_t size) {
    auto it = global_channel_handlers.find(type);
    if (it == global_channel_handlers.end()) {
        return false;
    }

    const GlobalChannelHandler &channel_handler = it->second;
    if (size > channel_handler.max_data_size) {
        throw std::runtime_error("Data size exceeds max_data_size");
    }
    channel_handler.on_data(channel_handler.arg, src_id, data, size);

    return true;
}
