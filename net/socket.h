#pragma once

#include <unistd.h>

class SocketGuard {
    public:

    int fd = -1;

    inline SocketGuard(int fd) : fd(fd) {}
    inline SocketGuard() {}

    inline SocketGuard(const SocketGuard&) = delete;
    inline SocketGuard& operator=(const SocketGuard&) = delete;

    inline SocketGuard(SocketGuard&& other) {
        fd = other.fd;
        other.fd = -1;
    }

    inline void clear() {
        if (fd >= 0) {
            close(fd);
            fd = -1;
        }
    }

    inline SocketGuard& operator=(SocketGuard&& other) {
        clear();
        fd = other.fd;
        other.fd = -1;
        return *this;
    }

    inline operator int() const {
        return fd;
    }

    inline int get() const {
        return fd;
    }

    inline int handle() {
        auto r = fd;
        fd = -1;
        return r;
    }

    inline ~SocketGuard() {
        clear();
    }
};

void set_socket_blocking(int fd, bool blocking = true);
void set_socket_timeout(int fd, float timeout_sec);
