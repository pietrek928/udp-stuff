#pragma once

#include <cstdint>

typedef enum {
    PING = 0,
    GET_PEER_INFO = 1,
    GET_APP_INFO = 2,
    OPEN_CHANNEL = 3,
    CLOSE_CHANNEL = 4,
    CHANNEL_START_NUM = 1 << 16,
} MessageType;

typedef struct {
    uint32_t size;
    uint32_t type;
} MessageHeader_t;
