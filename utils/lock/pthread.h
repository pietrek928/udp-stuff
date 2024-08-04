#pragma once

#include "../assert.h"

#include <pthread.h>

/* compatibility pthread version */
class LockObjectPthread {
    pthread_mutex_t locker = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

    public:

    // MUST be used inside synchronized block
    void wait() {
        ASSERT_DBG(locked(), "Object MUST be locked to use wait()");
        pthread_cond_wait(&cond, &locker);
    }

    void notify(int n=1) {
        pthread_cond_signal(&cond);
    }

    void notifyAll() {
        pthread_cond_signal(&cond);
    }

    bool locked() {
        if (!pthread_mutex_trylock(&locker)) {
            pthread_mutex_unlock(&locker);
            return false;
        }
        return true;
    }

    void lock_c() {
        pthread_mutex_lock(&locker);
    }

    void unlock_c() {
        pthread_mutex_unlock(&locker);
    }

    // inline lock_holder_t lock() {
    //     return lock_holder_t(*this);
    // }
};
