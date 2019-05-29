#ifndef __NET_DRV_H_
#define __NET_DRV_H_

class Connection;

class NetDriver {
public:
    class NetAddr {
        public:
        virtual size_t hash() const {return 0;}
    };

    virtual void detach(const NetAddr &addr) {}
    virtual void attach(const NetAddr &addr, Connection *conn) {}
    virtual void push_data(uint8_t *data, size_t l) {}
    virtual void push_control(uint8_t *data, size_t l) {}
    virtual void send_all() {}
    virtual void recv_all() {}
};

#endif /* __NET_DRV_H_ */

