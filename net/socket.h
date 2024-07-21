#pragma once

#include <unistd.h>

typedef int socket_t;

class SocketGuard {
    public:

    socket_t fd = -1;

    inline SocketGuard(socket_t fd) : fd(fd) {}
    inline SocketGuard() {}

    inline SocketGuard(const SocketGuard&) = delete;
    inline SocketGuard& operator=(const SocketGuard&) = delete;

    inline SocketGuard(SocketGuard&& other) {
        fd = other.handle();
    }

    inline void clear() {
        if (fd >= 0) {
            close(fd);
            fd = -1;
        }
    }

    inline SocketGuard& operator=(SocketGuard&& other) {
        clear();
        fd = other.handle();
        return *this;
    }

    inline operator int() const {
        return fd;
    }

    inline socket_t get() const {
        return fd;
    }

    inline socket_t handle() {
        auto r = fd;
        fd = -1;
        return r;
    }

    inline ~SocketGuard() {
        clear();
    }
};

void set_socket_blocking(socket_t fd, bool blocking = true);
void set_socket_timeout(socket_t fd, float timeout_sec);
