#include "handler.h"
#include "conn/exception.h"
#include "utils/common.h"

#include <cstdint>
#include <stdexcept>
#include <unordered_map>

// TODO: add some locking here

std::unordered_map<uint32_t, GlobalChannelHandler> global_channel_handlers;

void register_global_handler(uint32_t type, GlobalChannelHandler &&handler) {
    auto emplace_result = global_channel_handlers.emplace(type, std::move(handler));
    if (!emplace_result.second) {
        throw std::runtime_error("Handler already registered");
    }
}

bool global_handle_data(const PeerId512_t &src_id, uint32_t type, const byte_t *data, uint32_t size) {
    auto it = global_channel_handlers.find(type);
    if (it == global_channel_handlers.end()) {
        return false;
    }

    const GlobalChannelHandler &channel_handler = it->second;
    if (unlikely(size > channel_handler.max_data_size)) {
        throw ProtocolError(
            std::string("Data size ") + std::to_string(size)
            + " exceeds maximum " + std::to_string(channel_handler.max_data_size)
        );
    }
    channel_handler.on_data(channel_handler.arg, src_id, data, size);

    return true;
}
