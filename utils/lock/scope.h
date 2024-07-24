#pragma once

#include <cstddef>


template<class Tm, class Targ>
class ScopeLockArg {
    Tm *m;
    Targ arg;

    public:
        ScopeLockArg(ScopeLockArg &&l) = delete;
        inline void operator=(ScopeLockArg &&l) = delete;

        inline ScopeLockArg(Tm &_m, Targ arg)
            : m(&_m), arg(arg) {
            m->lock_c(arg);
        }

        inline ~ScopeLockArg() {
            m->unlock_c(arg);
        }
};

template<class Tm, class Targ>
class ScopeLockArgMovable {
    Tm *m;
    Targ arg;

    inline void clear() {
        if (likely(m)) {
            m->unlock_c(arg);
        }
    }

    public:
        inline ScopeLockArgMovable(ScopeLockArgMovable &&l)
            : m(l->m), arg(l->arg) {
            l->m = NULL;
        }

        inline void operator=(ScopeLockArgMovable &&l) {
            clear();
            m = l->m;
            arg = l->arg;
            l->m = NULL;
        }

        inline ScopeLockArgMovable(Tm &_m, Targ arg)
            : m(&_m), arg(arg) {
            m->lock_c(arg);
        }

        inline ~ScopeLockArgMovable() {
            m->unlock_c(arg);
        }
};

template<class Tm>
class ScopeLock {
    Tm *m;

    public:
        ScopeLock(ScopeLock &&l) = delete;
        void operator=(ScopeLock &&l) = delete;

        inline ScopeLock(Tm &_m)
            : m(&_m) {
            m->lock_c();
        }

        inline ~ScopeLock() {
            m->unlock_c();
        }
};

template<class Tm>
class ScopeLockMovable {
    Tm *m;

    inline void clear() {
        if (likely(m)) {
            m->unlock_c();
        }
    }

    public:
        ScopeLockMovable(ScopeLockMovable &&l) : m(l->m) {
            l->m = NULL;
        }

        inline void operator=(ScopeLockMovable &&l) {
            clear();
            m = l->m;
            l->m = NULL;
        }

        inline ScopeLockMovable(Tm &_m)
            : m(&_m) {
            m->lock_c();
        }

        inline ~ScopeLockMovable() {
            clear();
        }
};
