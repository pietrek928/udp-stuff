#ifndef __DEC_H_
#define __DEC_H_

#include "buffer.h"

class Decryptor {
    virtual void load_state(DataLoader &d);
    virtual void store_state(DataStorer &d);
    virtual void dec(std::vector<bytes> &out, const void *in, size_t l);
    virtual void dec_flush(std::vector<bytes> &out, const void *in, size_t l);
    virtual size_t block_size();
};

template<class DecryptorCore>
class BlockDecryptor : Decryptor {
    static constexpr size_t block_size = sizeof(BlockEncryptor::block_t);

    EncryptorCore core;

    PtrInBuf in;

    StaticOutBuf<block_size> plain_buf;
    StaticInBuf<block_size> enc_buf;
    
    void load_state(DataLoader &d) {
        core.load_state(d);
    }

    void store_state(DataLoader &d) {
        core.store_state(d);
    }

    inline void _enc(auto &out) {
        if (UNLIKELY(plain_buf.left())) {
            buf_cp(in, plain_buf);
        }
        
        if (LIKELY(in.left() >= block_size && out.left() >= block_size)) { // ???
            if (UNLIKELY(!enc_buf.empty())) {
                buf_cp(enc_buf, out);
                enc_buf.clear();
            }
            if (UNLIKELY(plain_buf.full())) {
                plain_buf.clear();
                core.enc_block(out.out_pos_ptr(), plain_buf.in_pos_ptr());
                out.shift(block_size);
            }

            auto encl = size_clamp(block_size, min(in.left(), out.left()) - block_size); // !!!!!
            core.enc((EncryptorCore::block_t*)out.out_pos_ptr(), (const EncryptorCore::block_t*)in.in_pos_ptr(), encl);
            in.shift(encl);
            out.shift(encl);

            core.enc_block(in.in_pos_ptr(), enc_block.out_pos_ptr());
            enc_buf.shift(block_size);
            in.shift(block_size);

            buf_cp(in, plain_buf);
            return out.left();
        } 

        buf_cp(in, plain_buf);
        if (!in_buf.left()) {
            buf_cp(enc_buf, out);
            if (!enc_buf.left()) {
                core.enc_block(out_buf.start_ptr(), plain_buf.in_start_ptr());
                plain_buf.clear();
            }
        }

    }

    virtual size_t enc(byte_t *out_ptr, size_t l) {
        PtrOutBuf out(out_ptr, l);

        _enc(out);

        return out.left();
    }

    virtual size_t enc_flush(byte_t *out_ptr, size_t l) {
        PtrOutBuf out(out_ptr, l);

        if (UNLIKELY(in.left())) {
            _enc(out);
        }

        if (UNLIKELY(!enc_buf.full())) {
            throw ;
        }

        //

        return l;
    }

    size_t block_size() {
        return EncryptorCore::block_size;
    }
};

#endif /* __DEC_H_ */

