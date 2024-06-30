#include "common.h"
#include <cstring>
#include <exception>
#include <netinet/in.h>
#include <string>

#include <time.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>

class ConnectionError : public std::exception {
    std::string descr;
    public:
    ConnectionError(std::string descr) : descr(descr) {}
};

void __conn_err(const char *descr, const char * cmd, int ret_val) {
    throw ConnectionError(
        ((std::string)cmd) + ": " + descr + " returned "
        + std::to_string(ret_val) + ", errno: " + strerror(errno)
    );
}

inline void _ccall(const char *descr, const char *cmd, int ret_val) {
    if (unlikely(ret_val < 0)) {
        __conn_err(descr, cmd, ret_val);
    }
}

#define ccall(descr, cmd) _ccall(descr, __AT__ " " #cmd, cmd)

void set_socket_blocking(int fd, bool blocking = true) {
    int flags = fcntl(fd, F_GETFL, 0);
    int old_flags = flags;
    ccall("getting flags", flags);
    if (blocking) {
        flags &= ~O_NONBLOCK;
    } else {
        flags |= O_NONBLOCK;
    }
    if (flags != old_flags) {
        ccall("setting flags", fcntl(fd, F_SETFL, flags));
    }
}

void set_socket_timeout(int fd, int timeout_sec, int timeout_usec = 0) {
    timeval tv;
    tv.tv_sec = timeout_sec;
    tv.tv_usec = timeout_usec;
    ccall("setting timeout", setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)));
    ccall("setting timeout", setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)));
}

void sleep_sec(float sec) {
    if (sec <= 0) {
        return;
    }
    timespec ts;
    ts.tv_sec = (time_t)sec;
    ts.tv_nsec = (long)((sec - ts.tv_sec) * 1e9);
    // TODO: sleep loop ?
    nanosleep(&ts, NULL);
}

float timespec_diff_sec(const timespec &start, const timespec &end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9;
}

timespec timespec_timestamp() {
    timespec ts;
    // TODO: error handling?
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts;
}

class SocketGuard {
    public:

    int fd = -1;

    SocketGuard(int fd) : fd(fd) {}
    SocketGuard() {}

    SocketGuard(const SocketGuard&) = delete;
    SocketGuard& operator=(const SocketGuard&) = delete;

    SocketGuard(SocketGuard&& other) {
        fd = other.fd;
        other.fd = -1;
    }

    void clear() {
        if (fd >= 0) {
            close(fd);
            fd = -1;
        }
    }

    SocketGuard& operator=(SocketGuard&& other) {
        clear();
        fd = other.fd;
        other.fd = -1;
        return *this;
    }

    operator int() const {
        return fd;
    }

    int get() const {
        return fd;
    }

    int handle() {
        auto r = fd;
        fd = -1;
        return r;
    }

    ~SocketGuard() {
        clear();
    }
};

int tcpv4_new_socket(bool blocking = true) {
    int flags = blocking ? 0 : SOCK_NONBLOCK;
    int fd = socket(AF_INET, SOCK_STREAM | flags, 0);
    ccall("creating socket", fd);
    return fd;
}

void tcpv4_close_socket(int fd) {
    ccall("closing socket", close(fd));
}

void tcpv4_bind_port(int fd, in_port_t src_port, in_addr_t src_addr = INADDR_ANY) {
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(src_port);
    addr.sin_addr.s_addr = htonl(src_addr);

    ccall("binding port", bind(fd, (struct sockaddr*)(&addr), sizeof(addr)));
}

void tcpv4_connect(int fd, in_port_t dst_port, in_addr_t dst_addr) {
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(dst_port);
    addr.sin_addr.s_addr = htonl(dst_addr);

    ccall("connecting", connect(fd, (struct sockaddr*)(&addr), sizeof(addr)));
}

void tcpv4_connect_abort(int fd) {
    sockaddr_in addr;
    addr.sin_family = AF_UNSPEC;
    ccall("aborting connection", connect(fd, (struct sockaddr*)(&addr), sizeof(addr)));
}

void tcpv4_listen(int fd, int backlog = 1) {
    ccall("listening", listen(fd, backlog));
}

typedef struct TCPv4AcceptResult {
    int new_fd;
    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
} TCPv4AcceptResult;

TCPv4AcceptResult tcpv4_accept(int fd, bool create_blocking = true) {
    int flags = create_blocking ? 0 : SOCK_NONBLOCK;
    TCPv4AcceptResult ret;
    ret.new_fd = accept4(fd, (struct sockaddr*)(&ret.addr), &ret.addr_len, flags);
    ccall("accepting", ret.new_fd);
    return ret;
}

typedef struct {
    in_port_t src_port;
    in_addr_t src_addr;
    in_port_t dst_port;
    in_addr_t dst_addr;

    time_t start_time;
    bool listen_first;
    float ping_sec;
    unsigned int connect_ping_count;
    unsigned int listen_ping_count;
    float connect_sec;

    unsigned int tries_count;
    float retry_sec;
} TCPv4HolePunchSettings;

int tcpv4_hole_punch(const TCPv4HolePunchSettings &settings) {
    auto tries = settings.tries_count;
    bool listen = settings.listen_first;
    do {
        auto start_timestamp = timespec_timestamp();

        SocketGuard main_fd = tcpv4_new_socket(false);
        tcpv4_bind_port(main_fd, settings.src_port, settings.src_addr || INADDR_ANY);

        unsigned int ping_count = listen ? settings.listen_ping_count : settings.connect_ping_count;
        do {
            tcpv4_connect(main_fd, settings.dst_port, settings.dst_addr);
            sleep_sec(settings.ping_sec);
            tcpv4_connect_abort(main_fd);
        } while (--ping_count);

        set_socket_blocking(main_fd, true);
        set_socket_timeout(main_fd, settings.connect_sec);
        if (listen) {
            tcpv4_listen(main_fd);
            auto res = tcpv4_accept(main_fd, true);
            if (res.new_fd >= 0) {
                // TODO: verify src ip here
                return res.new_fd;
            }
        } else {
            tcpv4_connect(main_fd, settings.dst_port, settings.dst_addr);
            return main_fd.handle();
        }

        auto end_timestamp = timespec_timestamp();
        sleep_sec(settings.connect_sec - timespec_diff_sec(start_timestamp, end_timestamp));
    } while (--tries);

    return -1;
}
