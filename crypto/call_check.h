#include "../utils/common.h"
#include "exception.h"

void __ssl_err(const char *descr);

inline void _scall(const char *descr, int ret_val) {
    if (unlikely(!ret_val)) {
        __ssl_err(descr);
    }
}

inline void _scall(const char *descr, void *ret_val) {
    if (unlikely(!ret_val)) {
        __ssl_err(descr);
    }
}

#define scall(descr, cmd) _scall(__AT__ " " #cmd ": " descr, cmd)
