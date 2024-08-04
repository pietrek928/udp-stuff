#pragma once

#include "../common.h"
#include "../assert.h"
#include "ops.h"

#include <climits>

/*
 * Faster for rarely conflicts ( typical case )
 * Lower memory footprint
 * */
template<
    int BRUTE_WAIT_LIMIT=24,
    int BRUTE_WAITERS_COUNT=1
>
class LockObjectBrute {
    futex_t _futex_var = 0;
    int num_futex_waiters = 0;
    int num_brute_waiters = 0;

    inline void futex_wait(futex_t start_val) {
        atomic_add(&num_futex_waiters, 1);
        futex(&_futex_var, FUTEX_WAIT, start_val);
        atomic_add(&num_futex_waiters, -1);
    }

    inline void futex_wait(futex_t start_val, float timeout_sec) {
        struct timespec timeout;
        timeout.tv_sec = (time_t)timeout_sec;
        timeout.tv_nsec = (long)((timeout_sec - timeout.tv_sec) * 1e9);

        atomic_add(&num_futex_waiters, 1);
        futex(&_futex_var, FUTEX_WAIT, start_val, &timeout);
        atomic_add(&num_futex_waiters, -1);
    }

    inline bool __brute_lock_wait(int limit=-1) {
        atomic_add(&num_brute_waiters, 1);
        do {
            if (atomic_try_lock(&_futex_var)) {
                atomic_add(&num_brute_waiters, -1);
                return true;
            }
            yield();
        } while (--limit);

        atomic_add(&num_brute_waiters, -1);
        return false;
    }

    void __lock_wait() {
        if (
            num_brute_waiters < BRUTE_WAITERS_COUNT
            && __brute_lock_wait(BRUTE_WAIT_LIMIT)
        ) {
            return;
        }
        do {
            futex_wait(1);
        } while (!atomic_try_lock(&_futex_var));
    }

    public:

    // MUST be used inside synchronized block
    void wait() {
        ASSERT_DBG(locked(), "Object MUST be locked to use wait()");
        unlock_c();
        futex_wait(0);
        lock_c();
    }

    int notify(int n=1) {
        if (num_futex_waiters) {
            return futex(&_futex_var, FUTEX_WAKE, n);
        }
        return 0;
    }

    inline void notifyAll() {
        notify(INT_MAX);
    }

    inline bool locked() {
        return (bool)_futex_var;
    }

    void lock_c() { // TODO: scope object ?
        if (unlikely(!atomic_try_lock(&_futex_var))) {
            __lock_wait();
        }
    }

    void unlock_c() {
        atomic_release(&_futex_var);
        if (!num_brute_waiters) {
            notify();
        }
    }
};
