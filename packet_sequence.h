#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <stdexcept>

template <class Tseq> class PacketSequence {
public:
  Tseq seq_start = 0;
  std::size_t data_size = 0;
  std::size_t mtu = 0;
  uint8_t *data = NULL;

  auto packet_start_pos(Tseq packet_seq) {
    Tseq packet_num = packet_seq - seq_start;
    return ((std::size_t)packet_num) * mtu;
  }

  PacketSequence(Tseq seq_start, std::size_t data_size, std::size_t mtu)
      : seq_start(seq_start), data_size(data_size), mtu(mtu) {
    data = (uint8_t *)malloc(data_size);
    if (!data) {
      throw std::bad_alloc();
    }
  }

  ~PacketSequence() { clear(); }

  auto packets_count() {
    auto cnt = data_size / mtu;

    if (data_size % mtu) {
      return cnt + 1;
    }
    return cnt;
  }

  auto packet_loc(Tseq packet_seq) {
    auto start_pos = packet_start_pos(packet_seq);
    if (start_pos >= data_size) {
      throw std::out_of_range("Invalid packet location");
    }

    struct {
      uint8_t *data;
      std::size_t size;
    } r;
    r.data = data + start_pos;
    r.size = std::min(data_size - start_pos, mtu);
    return r;
  }

  void clear() {
    if (data) {
      free(data);
      data = NULL;
    }
    data_size = 0;
    mtu = 0;
    seq_start = 0;
  }
};