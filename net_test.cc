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

        .listen_first = false,
        .connect_sec_start = .5,
        .connect_sec_max = 4,
        .connect_sec_scale = 1.5,
        .connect_count = 4,
    };
    inet_pton(AF_INET, "194.181.188.2", &settings.dst_addr);
    int sock = tcpv4_hole_punch(settings);
    printf("Socket: %d\n", sock);
    return 0;
}
