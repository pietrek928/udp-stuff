#include "net/tcpv4.h"
#include <arpa/inet.h>

#include <cstdio>

int main() {
    TCPv4HolePunchSettings settings = {
        .src_port = 12345,
        .src_addr = INADDR_ANY,
        .dst_port = 12345,
        .dst_addr = INADDR_ANY,
        .start_time = 0,
        .sequence_sec = 60,
        .tries_count = 2,
        .listen_first = false,
        .ping_sec = 3,
        .connect_ping_count = 2,
        .listen_ping_count = 2,
        .connect_sec = 4,
        .connect_count = 2,
        .listen_sec = 3,
        .listen_count = 3
    };
    inet_pton(AF_INET, "194.181.188.2", &settings.dst_addr);
    int sock = tcpv4_hole_punch(settings);
    printf("Socket: %d\n", sock);
    return 0;
}
