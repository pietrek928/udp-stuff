#include "assert.h"

namespace std {
    string to_string(const char *s) {
        return s;
    }
    string to_string(string &s) {
        return s;
    }
}
