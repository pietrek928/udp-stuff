#ifndef __BUFFER_H_
#define __BUFFER_H_

template<class T, class len_t = int>
class PtrBuf {
    T *buf_e;
    len_t l;

    public:

    PtrBuf(T *buf, len_t l)
        : buf_e(buf + l), l(l) {
    }

    inline void shift(len_t v) {
        l -= v;
    }

    inline auto empty() {
        return !l;
    }

    inline auto left() {
        return l;
    }

    inline auto start() {
        return buf_e - l;
    }

    inline auto end() {
        return buf_e;
    }

    inline void finish() {
        l = 0;
    }
};

template<size_t size, class T, class len_t = int>
class StaticBuf {
    T buf[size];
    len_t l;

    public:

    StaticBuf() {
    }

    inline auto buf_e() {
        return buf + size;
    }

    public:

    inline void shift(len_t v) {
        l += v;
    }

    inline auto empty() {
        return !l;
    }

    inline auto left() {
        return l;
    }

    inline auto start() {
        return buf_e() - l;
    }

    inline auto end() {
        return buf_e();
    }

    inline void finish() {
        l = 0;
    }
};

template<class Tq>
class InBufWrapper {
    Tq &q;

    public:
    InBufWrapper(Tq &q) : q(q) {}
    inline auto start() {
        return q.buf + q.e;
    }
    inline auto left() {
        if (q.b <= q.e) {
            return q.size - q.e;
        } else {
            return q.b - q.e - 1;
        }
    }
    inline auto finish() {
        if (q.b <= q.e) {
            q.e = 0;
        } else {
            q.e = q.b - 1;
        }
        // !!!
    }
    inline auto shift(len_t s) {
        q.e += s;
        if (q.e >= q.size) {
            q.e = 0;
        }
        // !!!
    }
};

template<class Tq>
class OutBufWrapper {
    Tq &q;

    public:
    InBufWrapper(Tq &q) : q(q) {}
    inline auto start() {
        return q.buf + q.b;
    }
    inline auto left() {
        if (q.b <= q.proc_e) {
            return q.proc_e - q.b;
        } else {
            return q.size - q.b;
        }
    }
    inline auto finish() {
        if (q.b <= q.proc_e) {
            q.b = q.proc_e;
        } else {
            q.b = 0;
        }
    }
    inline auto shift(len_t s) {
        q.b += s;
        if (q.b >= q.size) {
            q.b = 0;
        }
    }
};


template<size_t block_size, class T, class len_t = int>
class DoubleBlockProcBuf {
    public:
    constexpr static int size = block_size * 2;

    T buf[size];
    len_t b;
    len_t proc_e;
    len_t e;

    inline auto _left(len_t _b, len_t _e) {
        if (_b < _e) {  // TODO: mod & ?
            return _e - _b;
        } else {
            return size - _b;
        }
    }

    public:

    auto in() {
        class {
            //
            
        } r;
        return r;
    }

    void clear() {
        b = proc_e = e = 0;
    }
};

template<class Tbin, class Tbout>
inline void buf_cp(Tbin &in, Tbout &out) {
    if (in.left() < out.left()) {
        memcpy(out.start(), in.start(), in.left());
        out.shift(in.left());
        in.finish();
    } else {
        memcpy(out.start(), in.start(), out.left());
        in.shift(out.left());
        out.finish();
    }
}

typedef PtrBuf<const byte_t> PtrInBuf;
typedef PtrBuf<byte_t> PtrOutBuf;

template<size_t size>
typedef StaticBuf<size, const byte_t> StaticInBuf
template<size_t size>
typedef StaticBuf<size, byte_t> StaticOutBuf

#ifndef /* __BUFFER_H_ */

