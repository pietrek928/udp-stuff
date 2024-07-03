#pragma once

#include <cstddef>
#include <cstdint>

#define __STR(x) #x
#define STR(x) __STR(x)
#define __AT__ __FILE__ ":" STR(__LINE__)

#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)

typedef uint8_t byte_t;

template<class Tobj, void(*free_func)(Tobj*)>
class GuardPointer {
    public:

    Tobj *ptr = NULL;

    GuardPointer(Tobj *ptr) : ptr(ptr) {}
    GuardPointer() {}

    GuardPointer(const GuardPointer&) = delete;
    GuardPointer& operator=(const GuardPointer&) = delete;

    GuardPointer(GuardPointer&& other) {
        ptr = other.ptr;
        other.ptr = NULL;
    }

    GuardPointer& operator=(GuardPointer&& other) {
        if (ptr) {
            free_func(ptr);
        }
        ptr = other.ptr;
        other.ptr = NULL;
        return *this;
    }

    operator Tobj*() const {
        return ptr;
    }

    Tobj* get() const {
        return ptr;
    }

    Tobj *handle() {
        auto r = ptr;
        ptr = NULL;
        return r;
    }

    ~GuardPointer() {
        if (ptr) {
            free_func(ptr);
        }
    }
};
