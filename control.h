#pragma once

#include <cstdint>

#define PACKED __attribute__((packed, aligned(1)))

const uint32_t KEY_ANY = -1;

typedef enum {
    ERR_INFO = 9,
    SHIFT_RANGE_REQUEST = 10,
    SHIFT_RANGE_INFO = 11,
    DATA_SEQ_RANGE = 12,
    PUB_KEY = 13,
    SYM_KEY = 14,
    DATA_ENC_INFO = 15,
} control_t;

typedef enum {
    UNKNOWN_CONTROL = 0,
} err_t;

struct ControlStruct {
    inline uint32_t data_size() const { return 0; }
} PACKED;


/*** Communication control structures ***/

struct ErrInfo : public ControlStruct {
    err_t err;
} PACKED;

struct ShiftRangeRequest : public ControlStruct {
    uint16_t seq_start, seq_end;
} PACKED;

struct ShiftRangeInfo {
    uint16_t old_seq_start, old_seq_end;
    uint16_t new_seq_start, new_seq_end;
} PACKED;

struct DataSeqRange : public ControlStruct {
    uint16_t seq_start, seq_end;
} PACKED;

struct PubKey : public ControlStruct {
    uint32_t conn_key_id;
    uint32_t pubkey_type;
    uint32_t pubkey_size;
    uint8_t data[];

    inline uint32_t data_size() const { return pubkey_size; }
} PACKED;

struct SymKey : public ControlStruct {
    uint32_t conn_key_id;
    uint32_t symkey_type;
    uint32_t symkey_size;
    uint8_t data[];

    inline uint32_t data_size() const { return symkey_size; }
} PACKED;

struct DataEncInfo : public ControlStruct {
    uint32_t sym_key_id;
    uint32_t random_prefix_size;
    uint32_t sgn_key_id;
    uint32_t sgn_type;
    uint32_t sgn_size;
} PACKED;
