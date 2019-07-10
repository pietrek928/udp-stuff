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

