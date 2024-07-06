#pragma once

#include "../utils/common.h"
#include "exception.h"

#include <cstring>

void __conn_err(const char *descr, const char * cmd, int ret_val);

inline void _ccall(const char *descr, const char *cmd, int ret_val) {
    if (unlikely(ret_val < 0)) {
        __conn_err(descr, cmd, ret_val);
    }
}

#define ccall(descr, cmd) _ccall(descr, __AT__ " " #cmd, cmd)
