#ifndef __PACKET_BUFFER_H_
#define __PACKET_BUFFER_H_

#include <cstddef>
#include <cstdint>
#include <utility>

template <class Tpacket, class Tseq> class PacketBuffer {
  Tpacket *packets = NULL;
  uint32_t size;

  uint32_t start = 0, end = 0;
  Tseq seq_start = 0;

  struct IndexIter {
    PacketBuffer *buf;
    uint32_t pos_idx;
    uint32_t end_idx;

    IndexIter(PacketBuffer *buf, uint32_t pos_idx, uint32_t end_idx)
        : buf(buf), pos_idx(pos_idx), end_idx(end_idx) {}

    auto begin() { return *this; }
    auto end() { return end_idx; }

    auto operator!=(uint32_t end_idx_cmp) { return end_idx != end_idx_cmp; }

    auto &operator++() {
      pos_idx = buf->next_pos();
      return *this;
    }
  };

public:
  PacketBuffer(uint32_t size) : size(size) { packets = new Tpacket[size]; }
  PacketBuffer(PacketBuffer &&old_buffer, uint32_t new_size) : size(new_size) {
    packets = new Tpacket[size];
    seq_start = old_buffer.seq_start;
    end = old_buffer.length();

    auto old_packets = old_buffer.packets;
    if (old_buffer.start <= old_buffer.end) {
      for (uint32_t i = 0; i < end; i++) {
        packets[i] = std::move(old_packets[old_buffer.start + i]);
      }
    } else {
      auto start_size = old_buffer.size - old_buffer.start;
      for (uint32_t i = 0; i < start_size; i++) {
        packets[i] = std::move(old_packets[old_buffer.start + i]);
      }
      for (uint32_t i = 0; i < old_buffer.start; i++) {
        packets[start_size + i] = std::move(old_packets[i]);
      }
    }
    old_buffer.clear();
  }

  ~PacketBuffer() { clear(); }

  auto &operator=(PacketBuffer &&m) {
    std::swap(packets, m.packets);
    std::swap(size, m.size);
    std::swap(start, m.start);
    std::swap(end, m.end);
    std::swap(seq_start, m.seq_start);
    m.clear();
  }

  auto length() {
    if (start <= end) {
      return end - start;
    } else {
      return size + end - start;
    }
  }

  auto next_pos(uint32_t pos) {
    pos++;
    if (pos < size) {
      return pos;
    } else {
      return 0;
    }
  }

  auto prev_pos(uint32_t pos) {
    if (pos) {
      return pos - 1;
    } else {
      return size - 1;
    }
  }

  auto iter() { return IndexIter(this, start, end); }

  auto seq_range(uint32_t seq1, uint32_t seq2) {
    auto pos1 = seq1 + (start - seq_start);
    auto pos2 = seq2 + (start - seq_start);
    return IndexIter(this, pos1, next_pos(pos2));
  }

  void resize(uint32_t new_size) {
    *this = PacketBuffer(std::move(*this), new_size);
  }

  void clear() {
    if (packets) {
      delete[] packets;
      packets = NULL;
    }
    size = 0;
    start = 0;
    end = 0;
    seq_start = 0;
  }
};

#endif /* __PACKET_BUFFER_H_ */
