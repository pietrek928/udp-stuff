#pragma once

#include "../utils/common.h"

#include <openssl/types.h>

// 512-bits id
typedef struct PeerId512_t {
    size_t id[512 / 8 / sizeof(size_t)] = {0};
} PeerId512_t;

#include <functional>

inline bool operator==(const PeerId512_t &id1, const PeerId512_t &id2) {
    auto data_len = sizeof(id1.id)/sizeof(std::size_t);

    for (int i=0; i<data_len; i++) {
        if (id1.id[i] != id2.id[i]) {
            return false;
        }
    }

    return true;
}

template <>
struct std::hash<PeerId512_t> {
  inline std::size_t operator()(const PeerId512_t& k) const {
    auto data_len = sizeof(k.id)/sizeof(std::size_t);

    std::size_t v = 0;
    for (int i=0; i<data_len; i++) {
        v += k.id[i];
    }
    return v;
  }
};

PeerId512_t peer_id_from_sha512(const byte_t *data, size_t data_len);
void ssl_add_verify_peer_id(SSL *ssl, const PeerId512_t *peer_id_ptr);
void ssl_clear_verify_peer_id(SSL *ssl);
