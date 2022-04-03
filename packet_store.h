#ifndef __PACKET_STORE_H_
#define __PACKET_STORE_H_

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>

class PacketStore {
public:
  void *data = NULL;
  uint32_t size = 0;

  PacketStore() {}

  PacketStore(const void *src_data, uint32_t size) : size(size) {
    if (size) {
      data = malloc(size);
      if (!data) {
        throw std::bad_alloc();
      }
      memcpy(data, src_data, size);
    }
  }

  ~PacketStore() { clear(); }

  void clear() {
    if (data) {
      free(data);
      data = NULL;
      size = 0;
    }
  }
};

#endif /* __PACKET_STORE_H_ */
