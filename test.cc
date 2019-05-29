#include <iostream>
#include <vector>

#include "err.h"
#include "buffer.h"
#include "config.h"
#include "udp4.h"

using namespace std;

//#define AA(a, args...) #a, AA(args)

void print_hex(unsigned char *buf, int n) {
    for (int i=1; i<=n; i++) {
        cout << setfill('0') << setw(2) << hex << (unsigned int)buf[i-1] << " ";
        if (!(i % 16)) cout << endl;
    }
    cout << endl << endl << dec;
}

template<class T>
T func() {
    return 0;
}

int main() {
    //int a = (int)func<>();
    ImmutableConfig c("src_ip ipv4:127.0.0.1; dst_ip ipv4:127.0.0.1; src_port 12; dst_port 15");
    for (int i=0; i<10000000; i++) {
        Udp4Driver::Addr a;
        a.load(&c);
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

