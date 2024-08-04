#pragma once

#include <unistd.h>
#include <sys/syscall.h>
#include <linux/futex.h>

typedef int futex_t;

inline int futex(
    futex_t *futex_var, int futex_op, futex_t val,
    const struct timespec *timeout = NULL, int *uaddr2 = NULL, int val3 = 0
) {
    return syscall(
        SYS_futex, futex_var, futex_op, val,
        timeout, uaddr2, val3
    );
}

inline void yield() {
    syscall(SYS_sched_yield);
}

inline int atomic_add(int *ptr, int val) {
    return __sync_add_and_fetch(ptr, val);
}

inline bool atomic_try_lock(int *ptr) {
    return !__sync_lock_test_and_set(ptr, 1);
}

inline void atomic_release(int *ptr) {
    __sync_lock_release(ptr);
}
