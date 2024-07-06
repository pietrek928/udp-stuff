#include "handler.h"

#include <cstdint>
#include <stdexcept>
#include <unordered_map>


std::unordered_map<uint32_t, ChannelHandler> channel_handlers;

void register_handler(uint32_t type, ChannelHandler handler) {
    if (channel_handlers.find(type) != channel_handlers.end()) {
        throw std::runtime_error("Handler already registered");
    }
    channel_handlers[type] = handler;
}

void unregister_handler(uint32_t type) {
    if (channel_handlers.find(type) == channel_handlers.end()) {
        throw std::runtime_error("Handler not registered");
    }
    channel_handlers.erase(type);
}

void handle_data(uint32_t type, const byte_t *data, uint32_t size) {
    auto channel_handler_it = channel_handlers.find(type);
    if (channel_handler_it == channel_handlers.end()) {
        throw std::runtime_error("Handler not found");
    }
    ChannelHandler channel_handler = channel_handler_it->second;
    if (size > channel_handler.max_data_size) {
        throw std::runtime_error("Data size exceeds max_data_size");
    }
    channel_handler.on_data(channel_handler.arg, data, size);
}
