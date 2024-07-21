#pragma once

#include "socket.h"

#include <netinet/in.h>

typedef struct TCPv4AcceptResult {
    socket_t new_fd;
    sockaddr_in addr;
} TCPv4AcceptResult;

typedef struct {
    in_port_t src_port;
    in_addr_t src_addr;
    in_port_t dst_port;
    in_addr_t dst_addr;

    time_t start_time;

    bool listen_first;
    float connect_sec_start;
    float connect_sec_max;
    float connect_sec_scale;
    unsigned int connect_count;
} TCPv4HolePunchSettings;

socket_t tcpv4_new_socket(bool blocking = true);
void tcpv4_close_socket(socket_t fd);
void tcpv4_bind_port(socket_t fd, in_port_t src_port, in_addr_t src_addr = INADDR_ANY);
void tcpv4_connect(socket_t fd, in_port_t dst_port, in_addr_t dst_addr);
void tcpv4_connect_abort(socket_t fd);
void tcpv4_listen(socket_t fd, int backlog = 1);
TCPv4AcceptResult tcpv4_accept(socket_t fd, bool create_blocking = true);
socket_t tcpv4_hole_punch(const TCPv4HolePunchSettings &settings);
