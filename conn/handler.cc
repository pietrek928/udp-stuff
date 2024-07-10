#include "handler.h"

#include <cstdint>
#include <stdexcept>
#include <vector>


std::vector<GlobalChannelHandler> global_channel_handlers;

uint32_t get_handlers_count() {
    return global_channel_handlers.size();
}

uint32_t register_handler(GlobalChannelHandler handler) {
    global_channel_handlers.push_back(handler);
    return global_channel_handlers.size() - 1;
}

void handle_data(const PeerId_t &src_id, uint32_t type, const byte_t *data, uint32_t size) {
    if (type >= global_channel_handlers.size()) {
        throw std::runtime_error("Handler not registered");
    }

    const GlobalChannelHandler &channel_handler = global_channel_handlers[type];
    if (size > channel_handler.max_data_size) {
        throw std::runtime_error("Data size exceeds max_data_size");
    }
    channel_handler.on_data(channel_handler.arg, src_id, data, size);
}
