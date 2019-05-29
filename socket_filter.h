#ifndef __SOCKET_FILTER_H_
#define __SOCKET_FILTER_H_

#include <linux/filter.h>

#define FIELD_POS(s, f) (size_t)(&(((s*)0)->f))
constexpr static int OP_LDH = (BPF_LD  | BPF_H   | BPF_ABS);
constexpr static int OP_LDB = (BPF_LD  | BPF_B   | BPF_ABS);
constexpr static int OP_SUB = (BPF_ALU | BPF_SUB | BPF_K);
constexpr static int OP_JEQ = (BPF_JMP | BPF_JEQ | BPF_K);
constexpr static int OP_JGT = (BPF_JMP | BPF_JGT | BPF_K);
constexpr static int OP_RET = (BPF_RET | BPF_K);
constexpr static unsigned int BPF_ACCEPT = ((unsigned int)-1);
constexpr static int BPF_DROP = 0;

void socket_attach_filter(int fd, struct sock_filter *bpfcode, short unsigned int len) {
    struct sock_fprog bpf = { len, bpfcode };
    scall(
        "Setting socket filter",
        setsockopt(fd, SOL_SOCKET, SO_ATTACH_FILTER, &bpf, sizeof(bpf))
    );
}


#endif /* __SOCKET_FILTER_H_ */

