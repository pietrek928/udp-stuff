#include <cstddef>
#include <stdexcept>
#include <new>
#include <cstdint>
#include <cstring>
#include <vector>
#include <map>

class PacketDescr {
    public:
    uint32_t size = 0;
    uint8_t *data = NULL;

    bool is_data() {
        return !(size & 1);
    }

    PacketDescr(uint32_t size, const uint8_t *data) : size(size) {
        if (size) {
            this->data = (uint8_t*)malloc(size);
            if (!this->data) {
                throw std::bad_alloc();
            }
            memcpy(this->data, data, size);
        }
    }
    PacketDescr(const PacketDescr &other) {
        PacketDescr(other.size, other.data);
    }
    PacketDescr(PacketDescr &&other) {
        size = other.size;
        data = other.data;
        other.data = NULL;
        other.size = 0;
    }
    ~PacketDescr() {
        clear();
    }
    void clear() {
        if (data) {
            free(data);
            data = NULL;
        }
    }
};

class PacketRecvQueue {
    public:

    uint16_t start_seq = 1, end_seq = 0; // seq 0 is omited
    std::map<uint16_t, PacketDescr> packets;
    std::map<uint16_t, uint16_t> seq_mapping;

    auto packets_count() {
        return packets.size();
    }

    auto packets_size() {
        uint32_t size = 0;
        for (auto &p : packets) {
            size += p.second.size;
        }
        return size;
    }

    auto data_packets_size() {
        uint32_t size = 0;
        for (auto &p : packets) {
            if (p.second.is_data()) {
                size += p.second.size;
            }
        }
        return size;
    }

    uint16_t resolve_seq(uint16_t seq) {
        auto it = seq_mapping.find(seq);
        while (it != seq_mapping.end()) {
            seq = it->second;
            it = seq_mapping.find(seq);
        }
        return seq;
    }

    void clear_range(uint16_t start, uint16_t end) {
        for (uint16_t i = start; i != end; i++) {
            auto itp = packets.find(i);
            if (itp != packets.end()) {
                itp->second.clear();
                packets.erase(itp);
            }

            auto its = seq_mapping.find(i);
            if (its != seq_mapping.end()) {
                seq_mapping.erase(its);
            }
        }
    }

    void move_seq(uint16_t new_seq) {
        clear_range(start_seq, new_seq);
        start_seq = new_seq;
    }

    uint32_t compute_range_size(uint16_t start, uint16_t end) {
        uint32_t size = 0;
        for (uint16_t i = start; i != end; i++) {
            uint16_t seq = resolve_seq(i);

            auto it = packets.find(seq);
            if (it != packets.end()) {
                if (it->second.is_data()) {
                    size += it->second.size;
                }
            } else {
                return -1;
            }
        }
        return size;
    }

    void find_missing_packets(uint16_t start, uint16_t end, std::vector<uint16_t> &missing) {
        for (uint16_t i = start; i != end; i++) {
            uint16_t seq = resolve_seq(i);

            auto it = packets.find(seq);
            if (it == packets.end()) {
                missing.push_back(i);
            }
        }
    }

    void push_packet(uint16_t seq, uint32_t size, const uint8_t *data) {
        // TODO: track duplicates, mapping here
        packets.emplace(seq, size, data);
    }

    void copy_data(uint8_t *data, uint16_t start, uint16_t end) {
        uint32_t offset = 0;
        for (uint16_t i = start; i != end; i++) {
            uint16_t seq = resolve_seq(i);

            auto it = packets.find(seq);
            if (it != packets.end()) {
                if (it->second.is_data()) {
                    memcpy(data + offset, it->second.data, it->second.size);
                    offset += it->second.size;
                }
            } else {
                throw std::runtime_error("Packet not found");
            }
        }
    }
};


class PacketSendQueue {
    public:

    uint32_t mtu = 1024;
    uint16_t start_seq = 1, end_seq = 0; // seq 0 is omited
    std::map<uint16_t, PacketDescr> packets;

    auto packets_count() {
        return packets.size();
    }

    auto packets_size() {
        uint32_t size = 0;
        for (auto &p : packets) {
            size += p.second.size;
        }
        return size;
    }

    auto data_packets_size() {
        uint32_t size = 0;
        for (auto &p : packets) {
            if (p.second.is_data()) {
                size += p.second.size;
            }
        }
        return size;
    }

    void clear_range(uint16_t start, uint16_t end) {
        for (uint16_t i = start; i != end; i++) {
            auto it = packets.find(i);
            if (it != packets.end()) {
                it->second.clear();
                packets.erase(it);
            }
        }
    }

    void push_packet(uint32_t size, const uint8_t *data) {
        packets.emplace(++end_seq, size, data);
    }

    // retransmission
    void move_packet_up(uint16_t seq) {
        uint16_t new_seq = ++end_seq;
        auto it = packets.find(seq);
        if (it != packets.end()) {
            packets.emplace(new_seq, std::move(it->second));
        }
    }

    void push_buffer(uint8_t *data, uint32_t size) {
        // TODO: assert size % 2 == 0
        uint32_t offset = 0;
        while (offset < size) {
            uint32_t packet_size = size - offset;
            if (packet_size > mtu) {
                packet_size = mtu;
            }
            push_packet(packet_size, data + offset);
            offset += packet_size;
        }
    }
};
