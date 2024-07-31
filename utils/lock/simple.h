#pragma once

#include "../common.h"
#include "../assert.h"
#include "scope.h"

#include <climits>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/futex.h>


class LockObjectSimple {
    using futex_t = int;

    futex_t _futex_var = 0;
    int num_futex_waiters = 0;

    inline int futex(
        int futex_op, futex_t val,
        const struct timespec *timeout, int *uaddr2, int val3
    ) {
        return syscall(
            SYS_futex, &_futex_var, futex_op, val,
            timeout, uaddr2, val3
        );
    }

    inline void yield() {
        syscall(SYS_sched_yield);
    }

    inline int add_to_futex_waiters(int n) {
        return __sync_add_and_fetch(&num_futex_waiters, n);
    }

    inline void futex_wait(futex_t start_val) {
        add_to_futex_waiters(1);
        futex(FUTEX_WAIT, start_val, NULL, NULL, 0);
        add_to_futex_waiters(-1);
    }

    inline void futex_wait(futex_t start_val, float timeout_sec) {
        struct timespec timeout;
        timeout.tv_sec = (time_t)timeout_sec;
        timeout.tv_nsec = (long)((timeout_sec - timeout.tv_sec) * 1e9);

        add_to_futex_waiters(1);
        futex(FUTEX_WAIT, start_val, &timeout, NULL, 0);
        add_to_futex_waiters(-1);
    }

    inline int atomic_try_lock() {
        return __sync_bool_compare_and_swap(&_futex_var, 0, 1);
    }

    void __lock_wait() {
        do {
            futex_wait(1);
        } while (!atomic_try_lock());
    }

    public:
    using lock_holder_t = ScopeLock<LockObjectSimple>;

    // MUST be used inside synchronized block
    void wait() {
        ASSERT_DBG(locked(), "Object MUST be locked to use wait()");
        unlock_c();
        futex_wait(0);
        lock_c();
    }

    int notify(int n=1) {
        if (num_futex_waiters) {
            return futex(FUTEX_WAKE, n, NULL, NULL, 0);
        }
        return 0;
    }

    inline void notifyAll() {
        notify(INT_MAX);
    }

    inline bool locked() {
        return (bool)_futex_var;
    }

    inline void lock_c() { // TODO: scope object ?
        if (unlikely(!atomic_try_lock())) {
            __lock_wait();
        }
    }

    inline void unlock_c() {
        __sync_lock_release(&_futex_var);
        notify();
    }

    inline lock_holder_t lock() {
        return lock_holder_t(*this);
    }
};