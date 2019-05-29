#ifndef __BUFFER_H_
#define __BUFFER_H_

#include <vector>
#include <exception>


class ChainedBuffer {
public:
    virtual void vector_insert(std::vector<uint8_t> &v) {
        throw std::runtime_error("Unimplemented");
    }
};

template<class Tstruct>
class StructBuffer : public ChainedBuffer {
    ChainedBuffer *parent;
public:

    Tstruct struct_;

    StructBuffer(ChainedBuffer &parent)
        : parent(&parent) {}

    void vector_insert(std::vector<uint8_t> &v) {
        if (parent) {
            parent->vector_insert(v);
        }
        v.insert(v.end(), (uint8_t*)&struct_, ((uint8_t*)&struct_) + sizeof(struct_));
    }
};

template<class Tstruct>
class StructInitBuffer : public ChainedBuffer {
public:

    Tstruct struct_;

    StructInitBuffer() {}

    void vector_insert(std::vector<uint8_t> &v) {
        v.insert(v.end(), (uint8_t*)&struct_, ((uint8_t*)&struct_) + sizeof(struct_));
    }
};

class VectorChainedBuffer : public ChainedBuffer {
    ChainedBuffer *parent;
public:

    std::vector<uint8_t> vv;

    template<class ... Targs>
    VectorChainedBuffer(ChainedBuffer &parent, Targs... vargs)
        : parent(&parent), vv(vargs...) {}

    void vector_insert(std::vector<uint8_t> &v) {
        parent->vector_insert(v);
        v.insert(v.end(), vv.begin(), vv.end());
    }
};

class ArrayChainedBuffer : public ChainedBuffer {
    ChainedBuffer *parent;
public:

    uint8_t *buf;
    size_t buf_size;

    ArrayChainedBuffer(ChainedBuffer &parent, uint8_t *buf, size_t buf_size)
        : parent(&parent), buf(buf), buf_size(buf_size) {}

    void vector_insert(std::vector<uint8_t> &v) {
        parent->vector_insert(v);
        v.insert(v.end(), buf, buf+buf_size);
    }
};

#endif /* __BUFFER_H_ */

