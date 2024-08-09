#include "ops.h"
#include "../common.h"


class RefObject {
    /* Assume we have 1 reference by constructor caller */
    int ref_count = 1;

    public:

    virtual int inc_ref() {
        return atomic_add(&ref_count, 1);
    }

    virtual int dec_ref() {
        return atomic_add(&ref_count, -1);
    }

    virtual ~RefObject() {}
};


template<class To>
class Ref {
    To *ptr = NULL;

    void inc_ref() {
        if (likely(ptr)) {
            ptr->inc_ref();
        }
    }

    public:

    Ref() {}

    Ref(To *ptr) : ptr(ptr) {
        inc_ref();
    }

    Ref(const Ref<To> &other) : ptr(other.ptr) {
        inc_ref();
    }

    Ref(Ref<To> &&other) : ptr(other.ptr) {
        other.ptr = nullptr;
    }

    void clear() {
        if (ptr) {
            auto refs_left = ptr->dec_ref();
            if (unlikely(!refs_left)) {
                delete ptr;
            }
            ptr = nullptr;
        }
    }

    Ref<To> &operator=(const Ref<To> &other) {
        clear();
        ptr = other.ptr;
        inc_ref();
        return *this;
    }

    Ref<To> &operator=(Ref<To> &&other) {
        clear();
        ptr = other.ptr;
        other.ptr = nullptr;
        return *this;
    }

    To *operator->() {
        return ptr;
    }

    const To *operator->() const {
        return ptr;
    }

    To &operator*() {
        return *ptr;
    }

    const To &operator*() const {
        return *ptr;
    }

    operator void*() const {
        return ptr;
    }

    ~Ref() {
        clear();
    }
};
