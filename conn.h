#ifndef __CONN_H_
#define __CONN_H_

#include "packet_buffer.h"
#include "packet_store.h"
#include <cstdint>
#include <vector>

template <class Tseq> class ConnectionStreamHandler {
  PacketBuffer<PacketStore, Tseq> in_packets;
  PacketBuffer<PacketStore, Tseq> out_packets;

  struct packet_store_t {
    PacketStore packet;
    uint16_t seq;
  };
  std::vector<packet_store_t> out_controls;

public:
  ConnectionStreamHandler() : in_packets(32), out_packets(32) {}
  ~ConnectionStreamHandler() {
    //
  }

  void process_control(Tseq seq, const void *data, uint32_t len) {
    //
  }
  void process_data(Tseq seq, const void *data, uint32_t len) {
    //
  }

  void disconnect() {
    //
  }
};

#endif /* __CONN_H_ */
