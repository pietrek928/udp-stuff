#include "call_check.h"

void __ssl_err(const char *descr) {
    throw OSSLError(descr);
}
