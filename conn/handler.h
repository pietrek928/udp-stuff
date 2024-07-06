#pragma once

#include "../utils/common.h"

#include <cstdint>


typedef struct ChannelHandler {
    uint32_t max_data_size;
    void (*on_data)(void *arg, const uint8_t *data, uint32_t size);
    void *arg = NULL;
} ChannelHandler;
