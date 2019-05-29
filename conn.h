#ifndef __CONN_H_
#define __CONN_H_

#include "net_drv.h"

typedef uint16_t packet_id_t;

class Connection {
    NetDriver *drv = NULL;
    const NetDriver::NetAddr *addr;

public:
    void disconnect();
    void connect(NetDriver *_drv, const NetDriver::NetAddr *addr);
    void process_control(packet_id_t id, uint8_t *data, size_t len);
    void process_data(packet_id_t id, uint8_t *data, size_t len);
    ~Connection();
};

#endif /* __CONN_H_ */

