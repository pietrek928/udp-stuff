#pragma once

#include <netinet/in.h>


typedef struct TCPv4AcceptResult {
    int new_fd;
    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
} TCPv4AcceptResult;

typedef struct {
    in_port_t src_port;
    in_addr_t src_addr;
    in_port_t dst_port;
    in_addr_t dst_addr;

    time_t start_time;
    float sequence_sec;
    unsigned int tries_count;

    bool listen_first;
    float ping_sec;
    unsigned int connect_ping_count;
    unsigned int listen_ping_count;
    float connect_sec;
    unsigned int connect_count;
    float listen_sec;
    unsigned int listen_count;
} TCPv4HolePunchSettings;

int tcpv4_new_socket(bool blocking = true);
void tcpv4_close_socket(int fd);
void tcpv4_bind_port(int fd, in_port_t src_port, in_addr_t src_addr = INADDR_ANY);
void tcpv4_connect(int fd, in_port_t dst_port, in_addr_t dst_addr);
void tcpv4_connect_abort(int fd);
void tcpv4_listen(int fd, int backlog = 1);
TCPv4AcceptResult tcpv4_accept(int fd, bool create_blocking = true);
int tcpv4_hole_punch(const TCPv4HolePunchSettings &settings);
