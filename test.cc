#include <iostream>
#include <iterator>
#include <vector>
#include <iomanip>
#include <unordered_map>

#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <linux/filter.h>
#include <linux/ip.h>
#include <linux/udp.h>

#include "err.h"
#include "buffer.h"

using namespace std;


#define FIELD_POS(s, f) (size_t)(&(((s*)0)->f))
constexpr static int OP_LDH = (BPF_LD  | BPF_H   | BPF_ABS);
constexpr static int OP_LDB = (BPF_LD  | BPF_B   | BPF_ABS);
constexpr static int OP_SUB = (BPF_ALU | BPF_SUB | BPF_K);
constexpr static int OP_JEQ = (BPF_JMP | BPF_JEQ | BPF_K);
constexpr static int OP_JGT = (BPF_JMP | BPF_JGT | BPF_K);
constexpr static int OP_RET = (BPF_RET | BPF_K);
constexpr static unsigned int BPF_ACCEPT = ((unsigned int)-1);
constexpr static int BPF_DROP = 0;

void socket_attach_filter(int fd, struct sock_filter *bpfcode, short unsigned int len) {
    struct sock_fprog bpf = { len, bpfcode };
    scall(
        "Setting socket filter",
        setsockopt(fd, SOL_SOCKET, SO_ATTACH_FILTER, &bpf, sizeof(bpf))
    );
}

template<class Tn>
auto hash_prepv(Tn v, int m) {
    return (v*m) | 1;
}

#include "conn.h"

class SendPacketQueue { // TODO: locks
    int mtu = 1500;
 
    vector<size_t> msg_sizes;
    vector<uint8_t> packet_buffers;

    public:

    void push(ChainedBuffer &packet) {
        auto old_size = packet_buffers.size();
        packet.vector_insert(packet_buffers);
        auto new_size = packet_buffers.size();
        msg_sizes.push_back(new_size - old_size);
    }

    void send(int sock) { // TODO: lock
        auto n = msg_sizes.size();
        vector<struct iovec> iov(n);
        vector<struct mmsghdr> messages(n);

        auto *data = &* packet_buffers.begin();
        for (int i=0; i<n; i++) {
            messages[i].msg_hdr.msg_iov = &iov[i];
            messages[i].msg_hdr.msg_iovlen = 1;
            iov[i].iov_base = data;
            iov[i].iov_len = msg_sizes[i];
            data += msg_sizes[i];
        }

        size_t sent = 0;
        do {
            auto l = sendmmsg(sock, (&*messages.begin()) + sent, n - sent, 0);
            if (l > 0) {
                sent += l;
            } else {
                // TODO: detect error
            }
        } while (sent < n);

        clear();
    }

    void clear() {
        msg_sizes.clear();
        packet_buffers.empty();
    }
};

namespace std {
    template <class T>
    struct hash<const T> {
        size_t operator()(const T &v) const {
            return v.hash();
        }
    };
}

class Udp4Driver : NetDriver { 

public:
    class Addr : public NetAddr {
        public:
        uint32_t src_ip;
        uint32_t dst_ip;
        uint16_t src_port;
        uint16_t dst_port;

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

    vector<struct iovec> iov;
    vector<struct mmsghdr> messages;
    vector<uint8_t> packet_buffers;

    SendPacketQueue send_queue; // TODO: double buffer ?

    unordered_map<const Addr, Connection*> conn_map;

    void filter_udp_port(int fd, uint16_t port) {
        struct sock_filter bpfcode[] = {
            BPF_STMT(OP_LDH, sizeof(iphdr) + FIELD_POS(udphdr, dest) ),	// load dest port
            BPF_JUMP(OP_JEQ, port, 1, 0), // compare dest port
            BPF_STMT(OP_RET, BPF_DROP),
            BPF_STMT(OP_RET, BPF_ACCEPT)
        };
        socket_attach_filter(fd, bpfcode, size(bpfcode));
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
        socket_attach_filter(fd, bpfcode, size(bpfcode));
    }

    inline void send_packet(const NetAddr &_addr, packet_id_t id, uint8_t *data, size_t l, bool is_control) {
        auto & addr = dynamic_cast<const Addr &>(_addr);

        StructInitBuffer<tuple<struct iphdr, struct udphdr>> hdr;
        auto & iph = get<0>(hdr.struct_);
        auto & udph = get<1>(hdr.struct_);

        iph.ihl = 5;
        iph.version = 4;
        iph.tot_len = htonl(sizeof(hdr.struct_) + l);
        iph.id = htonl(id);
        iph.ttl = 128; // TODO: config
        iph.protocol = IPPROTO_UDP;
        iph.saddr = addr.src_ip;
        iph.daddr = addr.dst_ip;
        //iph.check = 0;

        udph.source = addr.src_port;
        udph.dest = addr.dst_port;
        udph.len = htonl(sizeof(udph) + l);
        //udph.check = 0;

        ArrayChainedBuffer packet_buf(hdr, data, l);
        send_queue.push(packet_buf);
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
                sock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP)
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

        filter_udp_port(sock, 5550, 5560);

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
            cout << "aaaaaa" << endl;
            if (conn_fnd == conn_map.end()) continue; // TODO: support connect

            auto *conn = conn_fnd->second;
            if (!(iph->frag_off & 0x2000)) { // data packet or control packet
                conn->process_data(iph->id, data, data_len);
            } else {
                conn->process_control(iph->id, data, data_len);
            }
        }
    }
};

void print_hex(unsigned char *buf, int n) {
    for (int i=1; i<=n; i++) {
        cout << setfill('0') << setw(2) << hex << (unsigned int)buf[i-1] << " ";
        if (!(i % 16)) cout << endl;
    }
    cout << endl << endl << dec;
}

//#define AA(a, args...) #a, AA(args)
#include "config.h"

template<class T>
T func() {
    return 0;
}

int main() {
    //int a = (int)func<>();
    ImmutableConfig c("a 111;b{w;};o o o");
    int b = 0;
    for (int i=0; i<10000000; i++) {
        b += c.get<int>("a");
    }
    cout << b << endl;
    //c.parse("a a; a a; a a; a a");
    c.dump();
    //cout << AA(1, 2) << endl;

    //std::string text = "Let me split this into words";
    //auto splitText = text | view::split(' ');
    return 0;

    Udp4Driver u;
    u.init_socket();
    u.resize_buffers(8);

    while (1) {
        u.recv_all();
    }

    return 0;
}

#include "conn.cc"

