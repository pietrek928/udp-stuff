#ifndef __NET_DRV_H_
#define __NET_DRV_H_


namespace std {
    template <class T>
    struct hash<const T> {
        size_t operator()(const T &v) const {
            return v.hash();
        }
    };
}

class Connection;

class NetDriver {
public:
    class NetAddr {
        public:
        virtual size_t hash() const {return 0;}
        virtual void load(Config *config) {}
    };

    virtual void detach(const NetAddr &addr) {}
    virtual void attach(const NetAddr &addr, Connection *conn) {}
    virtual void push_data(uint8_t *data, size_t l) {}
    virtual void push_control(uint8_t *data, size_t l) {}
    virtual void send_all() {}
    virtual void recv_all() {}
};

#endif /* __NET_DRV_H_ */

