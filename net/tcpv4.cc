#include "tcpv4.h"

#include "call_check.h"
#include "../utils/time.h"
#include "exception.h"

#include <sys/socket.h>
#include <unistd.h>

socket_t tcpv4_new_socket(bool blocking) {
    int flags = blocking ? 0 : SOCK_NONBLOCK;
    socket_t fd = socket(AF_INET, SOCK_STREAM | flags, 0);
    ccall("creating socket", fd);
    return fd;
}

void tcpv4_close_socket(socket_t fd) {
    ccall("closing socket", close(fd));
}

void tcpv4_bind_port(socket_t fd, in_port_t src_port, in_addr_t src_addr) {
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = src_port;
    addr.sin_addr.s_addr = src_addr;

    ccall("binding port", bind(fd, (struct sockaddr*)(&addr), sizeof(addr)));
}

void tcpv4_connect(socket_t fd, in_port_t dst_port, in_addr_t dst_addr) {
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = dst_port;
    addr.sin_addr.s_addr = dst_addr;

    ccall("connecting", connect(fd, (struct sockaddr*)(&addr), sizeof(addr)));
}

int tcpv4_connect_unsafe(socket_t fd, in_port_t dst_port, in_addr_t dst_addr) {
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = dst_port;
    addr.sin_addr.s_addr = dst_addr;

    return connect(fd, (struct sockaddr*)(&addr), sizeof(addr));
}

void tcpv4_connect_abort(socket_t fd) {
    sockaddr_in addr;
    addr.sin_family = AF_UNSPEC;
    ccall("aborting connection", connect(fd, (struct sockaddr*)(&addr), sizeof(addr)));
}

void tcpv4_listen(socket_t fd, int backlog) {
    ccall("listening", listen(fd, backlog));
}

TCPv4AcceptResult tcpv4_accept(socket_t fd, bool create_blocking) {
    int flags = create_blocking ? 0 : SOCK_NONBLOCK;
    TCPv4AcceptResult ret;
    ret.new_fd = accept4(fd, (struct sockaddr*)(&ret.addr), &ret.addr_len, flags);
    ccall("accepting", ret.new_fd);
    return ret;
}

TCPv4AcceptResult tcpv4_accept_unsafe(socket_t fd, bool create_blocking) {
    int flags = create_blocking ? 0 : SOCK_NONBLOCK;
    TCPv4AcceptResult ret;
    ret.new_fd = accept4(fd, (struct sockaddr*)(&ret.addr), &ret.addr_len, flags);
    return ret;
}

socket_t tcpv4_hole_punch(const TCPv4HolePunchSettings &settings) {
    auto connect_tries = settings.connect_count;
    bool listen = settings.listen_first;
    float connect_sec = settings.connect_sec_start;
    do {
        auto start_timestamp = timespec_timestamp();

        SocketGuard main_fd = tcpv4_new_socket(true);
        tcpv4_bind_port(main_fd, settings.src_port, settings.src_addr || INADDR_ANY);
        set_socket_timeout(main_fd, connect_sec);

        if (listen) {
            tcpv4_listen(main_fd);
            auto res = tcpv4_accept_unsafe(main_fd, true);
            if (res.new_fd >= 0) {
                // TODO: verify src ip here
                return res.new_fd;
            }
        } else {
            if (!tcpv4_connect_unsafe(main_fd, settings.dst_port, settings.dst_addr)) {
                return main_fd.handle();
            }
        }

        auto end_timestamp = timespec_timestamp();
        sleep_sec(connect_sec - timespec_diff_sec(start_timestamp, end_timestamp));

        listen = !listen;
        connect_sec *= settings.connect_sec_scale;
        if (connect_sec > settings.connect_sec_max) {
            connect_sec = settings.connect_sec_max;
        }
    } while (--connect_tries);

    throw ConnectionError("TCPv4 hole punching failed");
}
