#pragma once

#include "../utils/common.h"

#include <cstddef>
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

// 512-bits id
typedef struct {
    byte_t id[64];
} PeerId_t;

#include <unordered_map>

template <>
struct std::hash<PeerId_t>
{
  std::size_t operator()(const PeerId_t& k) const
  {
    const std::size_t *data_ptr = (std::size_t *)&k.id;

    std::size_t v = 0;
    for (int i=0; i<sizeof(PeerId_t)/sizeof(std::size_t); i++) {
        v += data_ptr[i];
    }
    return v;
  }
};
