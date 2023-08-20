#include<arpa/inet.h>
#include <cstdint>
#include<sys/socket.h>

#include "packet_queue.h"


class UDP4Connection {
    PacketRecvQueue recv_queue;
    PacketSendQueue send_queue;

    struct sockaddr_in si_peer;

    public:
    UDP4Connection(
        const struct sockaddr_in &si_peer,
        const uint8_t *init_packet,
        uint32_t init_packet_size
    ) : si_peer(si_peer) {
        setup_from_init_packet(init_packet, init_packet_size);
    }

    void setup_from_init_packet(
        const uint8_t *init_packet,
        uint32_t init_packet_size
    ) {
        //
    }

    void process_control(const uint8_t *packet, uint32_t packet_size) {
        //
    }

    bool recv_packet(int socket, uint8_t *packet_buf, uint32_t buf_size) {
        int slen = sizeof(si_peer);
        auto result = recvfrom(
            socket, packet_buf, buf_size, 0, 
            (struct sockaddr*)&si_peer, (socklen_t*)&slen
        );
        if (result < 0) {
            throw std::runtime_error("recvfrom() failed");
        }
        if (result > 0) {
            uint16_t id = *(uint16_t*)packet_buf;
            recv_queue.push_packet(id, packet_buf + 2, result - 2);
            return true;
        }
        return false;
    }

    void send_packet(int socket, uint8_t *packet_buf, uint16_t id, uint8_t *data, uint32_t size) {
        *(uint16_t*)packet_buf = id;
        memcpy(packet_buf + 2, data, size);

        int result = sendto(
            socket, packet_buf, size + 2, 0, 
            (struct sockaddr*)&si_peer, sizeof(si_peer)
        );
        if (result < 0) {
            throw std::runtime_error("sendto() failed");
        }
    }

};