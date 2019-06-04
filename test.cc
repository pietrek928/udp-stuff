#include <iostream>
#include <iomanip>
#include <vector>

void print_hex(unsigned char *buf, int n) {
    for (int i=1; i<=n; i++) {
        std::cout << std::setfill('0') << std::setw(2) << std::hex << (unsigned int)buf[i-1] << " ";
        if (!(i % 16)) std::cout << std::endl;
    }
    std::cout << std::endl << std::endl << std::dec;
}



#include "err.h"
#include "buffer.h"
#include "config.h"
#include "udp4.h"
#include "sgn.h"

using namespace std;

//#define AA(a, args...) #a, AA(args)

template<class T>
T func() {
    return 0;
}

int main() {
    //int a = (int)func<>();
    //Config c1("src_ip ipv4:127.0.0.1; dst_ip ipv4:127.0.0.1; src_port 12; dst_port 15");
    //Config c2("src_ip ipv4:127.0.0.1; dst_ip ipv4:127.0.0.1; src_port 15; dst_port 12");
    Config c1("src_ip ipv4:192.168.1.104; dst_ip ipv4:192.168.1.104; src_port 12; dst_port 15");
    Config c2("src_ip ipv4:192.168.1.104; dst_ip ipv4:192.168.1.104; src_port 15; dst_port 12");
    //for (int i=0; i<10000000; i++) {
        //Udp4Driver::Addr a;
        //a.load(&c);
    //}
    Udp4Driver::Addr a1, a2;
    a1.load(&c1);
    a2.load(&c2);

    Connection conn1, conn2;
    Udp4Driver drv;

    conn1.connect(&drv, &a1);
    conn2.connect(&drv, &a2);

    drv.init_socket();
    drv.resize_buffers(8);

    drv.push_control(a1, 1, (uint8_t*)"aaaa", 4);
    drv.send_all();

    cout << "ready." << endl;

    while (1) {
        drv.recv_all();
    }

    /*ImmutableConfig c("r;r;r;r;r;r;r;r;;r;r;r;;;;rr;rr;r;;r;r;a 111;");
    int b = 0;
    for (int i=0; i<10000000; i++) {
        b += c.get<int>("a");
    }
    cout << b << endl;
    //c.parse("a a; a a; a a; a a");
    c.dump();*/
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

