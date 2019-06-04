#ifndef __UDP4_H_
#define __UDP4_H_

#include <unordered_map>

#include <linux/ip.h>
#include <linux/udp.h>

#include "net_drv.h"
#include "conn.h"
#include "socket_filter.h"

template<class Tn>
auto hash_prepv(Tn v, int m) {
    return (v*m) | 1;
}

class ChecksumCalculator {
    uint32_t sum = 0;

    public:
    ChecksumCalculator() {}

    template<class T>
    inline void add(const T *data, size_t l=sizeof(T)) {
        auto *buf = (const uint16_t *)data;
        auto *buf_e = buf + (l>>1);
        do {
            sum += *(buf++);
        } while (buf < buf_e);
        if (l & 1) {
            sum += *(uint8_t*)buf;
        }
    }

    inline uint16_t finish() {
        while (sum > 0xffff) {
            sum = (sum >> 16) + (sum & 0xFFFF);
        }
        return ~sum;
    }
};

class SendPacketQueue { // TODO: locks
    int mtu = 1500;

    struct PacketDescr {
        size_t len;
        struct sockaddr_in dst_addr;
    };
 
    std::vector<PacketDescr> msg_descr;
    std::vector<uint8_t> packet_buffers;

    public:

    void push(ChainedBuffer &packet, struct sockaddr_in &dst_addr) {
        auto old_size = packet_buffers.size();
        packet.vector_insert(packet_buffers);
        auto new_size = packet_buffers.size();

        auto &descr = msg_descr.emplace_back();
        descr.len = new_size - old_size;
        descr.dst_addr = dst_addr;
    }

    void send(int sock) { // TODO: lock
        auto n = msg_descr.size();
        std::vector<struct iovec> iov(n);
        std::vector<struct mmsghdr> messages(n);

        auto *data = &* packet_buffers.begin();
        for (int i=0; i<n; i++) {
            messages[i].msg_hdr.msg_iov = &iov[i];
            messages[i].msg_hdr.msg_iovlen = 1;
            messages[i].msg_hdr.msg_name = &msg_descr[i].dst_addr;
            messages[i].msg_hdr.msg_namelen = sizeof(msg_descr[i].dst_addr);
            iov[i].iov_base = data;
            iov[i].iov_len = msg_descr[i].len;
            print_hex(data, msg_descr[i].len);
            data += msg_descr[i].len;
        }

        size_t sent = 0;
        do {
            int l;
            scall(
                "Sending packets",
                (l = sendmmsg(sock, (&*messages.begin()) + sent, n - sent, 0)) >= 0
            );
            if (l > 0) {
                sent += l;
            } else {
                // TODO: detect error
            }
        } while (sent < n);

        clear();
    }

    void clear() {
        msg_descr.clear();
        packet_buffers.empty();
    }
};

class Udp4Driver : public NetDriver {

public:
    class Addr : public NetAddr {
        public:
        uint32_t src_ip;
        uint32_t dst_ip;
        uint16_t src_port;
        uint16_t dst_port;

        void load(const Config *cfg) {
            CONFIGURE(src_ip)
            CONFIGURE(dst_ip)
            CONFIGURE(src_port)
            CONFIGURE(dst_port)
            src_port = htons(src_port);
            dst_port = htons(dst_port);
        }

        size_t hash() const {
            return
                  ((size_t)hash_prepv(src_ip, 457))
                * hash_prepv(dst_ip, 765)
                * hash_prepv(src_port, 43)
                * hash_prepv(dst_port, 75);
        }

        bool operator==(const Addr &a2) const {
            return
                   src_ip == a2.src_ip
                && dst_ip == a2.dst_ip
                && src_port == a2.src_port
                && dst_port == a2.dst_port;
        }
    }; 

    int sock = -1;
    int mtu = 1500;

    std::vector<struct iovec> iov;
    std::vector<struct mmsghdr> messages;
    std::vector<uint8_t> packet_buffers;

    SendPacketQueue send_queue; // TODO: double buffer ?

    std::unordered_map<const Addr, Connection*> conn_map;

    void filter_udp_port(int fd, uint16_t port) {
        struct sock_filter bpfcode[] = {
            BPF_STMT(OP_LDH, sizeof(iphdr) + FIELD_POS(udphdr, dest) ),	// load dest port
            BPF_JUMP(OP_JEQ, port, 1, 0), // compare dest port
            BPF_STMT(OP_RET, BPF_DROP),
            BPF_STMT(OP_RET, BPF_ACCEPT)
        };
        socket_attach_filter(fd, bpfcode, std::size(bpfcode));
    }

    void filter_udp_port(int fd, uint16_t port_start, uint16_t port_end) {
        unsigned int port_range_len = port_end - port_start;
        struct sock_filter bpfcode[] = {
            BPF_STMT(OP_LDH, sizeof(iphdr) + FIELD_POS(udphdr, dest) ),	// load dest port
            BPF_STMT(OP_SUB, port_start),
            BPF_JUMP(OP_JGT, port_range_len, 0, 1), // compare dest port
            BPF_STMT(OP_RET, BPF_DROP),
            BPF_STMT(OP_RET, BPF_ACCEPT)
        };
        socket_attach_filter(fd, bpfcode, std::size(bpfcode));
    }

    inline void send_packet(const NetAddr &_addr, packet_id_t id, uint8_t *data, size_t l, bool is_control) {
        auto & addr = dynamic_cast<const Addr &>(_addr);
        struct Packet {
            struct iphdr ip;
            struct udphdr udp;
        };

        ArrayInitBuffer data_buf(data, l);
        StructBuffer<Packet> hdr(data_buf);
        auto & iph = hdr.struct_.ip;
        auto & udph = hdr.struct_.udp;

        iph.ihl = 5;
        iph.version = 4;
        iph.id = htons(id);
        iph.frag_off = is_control << 7;
        iph.ttl = 128; // TODO: config
        iph.protocol = IPPROTO_UDP;
        iph.saddr = addr.src_ip;
        iph.daddr = addr.dst_ip;

        udph.source = addr.src_port;
        udph.dest = addr.dst_port;
        udph.len = htons(sizeof(udph) + l);

        /* calculate udp checksum */
        ChecksumCalculator sum;
        sum.add(&iph.saddr);
        sum.add(&iph.daddr);
        uint16_t tmp_proto = htons(iph.protocol);
        sum.add(&tmp_proto);
        sum.add(&udph.len);
        sum.add(&udph);
        sum.add(data, l);
        udph.check = sum.finish();

        struct sockaddr_in dst_addr = {};
        dst_addr.sin_family = AF_INET;
        dst_addr.sin_addr.s_addr = iph.daddr;
        dst_addr.sin_port = udph.dest;

        send_queue.push(hdr, dst_addr);
    }

public:
    Udp4Driver() {}

    void detach(const NetAddr &addr) {
        conn_map.erase(
            dynamic_cast<const Addr &>(addr)
        );
    }

    void attach(const NetAddr &addr, Connection *conn) {
        conn_map.insert({
            dynamic_cast<const Addr &>(addr),
            conn
        });
    }

    void resize_buffers(int n) {
        iov.resize(n);
        messages.resize(n);
        packet_buffers.resize(n * mtu);

        for (int i=0; i<n; i++) {
            iov[i].iov_base = & packet_buffers[i * mtu];
            iov[i].iov_len = mtu;
            messages[i].msg_hdr.msg_iov = &iov[i];
            messages[i].msg_hdr.msg_iovlen=1;
        }
    }

    void init_socket() {
        scall("Creating socket", (
                sock = socket(PF_INET, SOCK_RAW, IPPROTO_UDP)
            ) != -1
        );

        /*int mtu_len = sizeof(mtu);
        scall(
            "Retrieving mtu",
            getsockopt(sock, IPPROTO_IP, IP_MTU, (char*)&mtu, (socklen_t*)(void*)&mtu_len)
        );*/

        int sock_flags;
        scall(
            "Retrieving current flags",
            (sock_flags = fcntl(sock, F_GETFL, 0)) != -1
        );
        scall(
            "Setting new flags",
            fcntl(sock, F_SETFL, sock_flags | O_NONBLOCK)
        );

        filter_udp_port(sock, 10, 50);

        int one = 1;
        scall(
            "Enabling including IP header to socket",
            setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one))
        );
    }

    void push_data(const NetAddr &addr, packet_id_t id, uint8_t *data, size_t l) {
        send_packet(addr, id, data, l, false);
    }

    void push_control(const NetAddr &addr, packet_id_t id, uint8_t *data, size_t l) {
        send_packet(addr, id, data, l, true);
    }

    void send_all() {
        send_queue.send(sock);
    }

    void recv_all() {
        int nreceived = recvmmsg(sock, &*messages.begin(), messages.size(), 0, NULL);
        for (int i=0; i<nreceived; i++) {
            auto &msg = messages[i];
            uint8_t *buf = (uint8_t*)msg.msg_hdr.msg_iov->iov_base;

            struct iphdr *iph = (struct iphdr *)buf;
            struct udphdr *udph = (struct udphdr *)(buf + iph->ihl*4);
            auto data = ((uint8_t*)udph) + sizeof(*udph);
            size_t data_len = htons(udph->len) - sizeof(*udph);

            // Something's wrong ?
            if (msg.msg_len != data - buf + data_len) continue;

            Addr addr;
            addr.src_ip = iph->daddr;
            addr.dst_ip = iph->saddr;
            addr.src_port = udph->dest;
            addr.dst_port = udph->source;

            auto conn_fnd = conn_map.find(addr);
            if (conn_fnd == conn_map.end()) continue; // TODO: support connect

            auto *conn = conn_fnd->second;
            if (!(iph->frag_off & 0x080)) { // data packet or control packet
                conn->process_data(htons(iph->id), data, data_len);
            } else {
                conn->process_control(htons(iph->id), data, data_len);
            }
        }
    }
};

#endif /* __UDP4_H_ */

