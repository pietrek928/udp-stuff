#ifndef __END_H_
#define __END_H_

/*template<size_t size, class pos_t=int>
class Buf {
    byte_t buf[len_t];
    pos_t pos;

    public:

    inline bool empty() {
        return !pos;
    }

    inline void put() {
    }
};*/

class Encryptor {
    virtual void load_state(DataLoader &d);
    virtual void store_state(DataStorer &d);
    virtual void enc(std::vector<bytes> &out, const void *in, size_t l);
    virtual void enc_flush(std::vector<bytes> &out, const void *in, size_t l);
    virtual size_t block_size();
};

template<class EncryptorCore>
class BlockEncryptor : Encryptor {
    static const size_t block_size = sizeof(BlockEncryptor::block_t);

    EncryptorCore core;

    byte_t *in_e;

    byte_t in_buf[block_size];
    byte_t out_buf[block_size];
    
    int in_size = 0;
    int in_buf_size = 0;
    int out_buf_size = 0;

    void load_state(DataLoader &d) {
        core.load_state(d);
    }

    void store_state(DataLoader &d) {
        core.store_state(d);
    }

    virtual size_t enc(byte_t *out, size_t l) {
        out += l;

_beg:;

        if (UNLIKELY(out_buf_size)) { // !!!!!!!!
            auto ncp = min(l, out_buf_size);
            memcpy(out-l, (out_buf+block_size) - out_buf_size, ncp);
            out_buf_size -= ncp;
            l -= ncp;

            if (UNLIKELY(out_buf_size)) {
                return l;
            }
        }

        if (UNLIKELY(in_buf_size)) {
            auto ncp = min(l, in_buf_size);
            memcpy(in_buf+in_buf_size, in+in_pos, ncp);
            in_buf_size += ncp;
            in_buf_size += ncp;

            if (LIKELY(in_buf_size == block_size)) {
                core.enc_block(out_buf, in_buf);
                in_buf_size = 0;
                out_buf_size = block_size;
                goto _beg;
            } else {
                return l;
            }
        }

        auto encl = size_clamp(block_size, min(in_size, l));
        core.enc((EncryptorCore::block_t*)(out-l), (const EncryptorCore::block_t*)(in_e-in_size), encl);

        l -= encl;
        in_size -= encl;

        if (UNLIKELY(l < block_size && l && in_size >= block_size)) {
            core.enc_block(out_buf, (const EncryptorCore::block_t*)(in_e-in_size));
            memcpy(in_e-l, out_buf, l);
            out_buf_size = block_size-l;
            l = 0;
        }

        if (UNLIKELY(in_size < block_size && in_size)) {
            memcpy(in_buf, in_e-in_size, in_size);
            in_buf_size = in_size;
            in_size = 0;
        }

        return l;
    }

    virtual void enc_flush(std::vector<bytes> &out, const void *in, size_t l) {
        size_t processed = core.enc((EncryptorCore::block_t*)out, (const EncryptorCore::block_t*)in, l);
        if (processed != l) {
            if (l < EncryptorCore::block_size) {
                throw EncryptorError("Encryption block too short");
            }
            memcpy(((const byte_t*)out) + processed, ((const byte_t*)in) + processed, l - processed);
            core.enc_block_inplace((EncryptorCore::block_t*)(((byte_t*)out) + (l - EncryptorCore::block_size)));
        }
        return l;
    }

    size_t block_size() {
        return EncryptorCore::block_size;
    }
};

#endif /* __END_H_ */

