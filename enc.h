#ifndef __END_H_
#define __END_H_

class Encryptor {
    virtual void load_state(DataLoader &d);
    virtual void store_state(DataStorer &d);
    virtual void enc(std::vector<bytes> &out, const void *in, size_t l);
    virtual void enc_flush(std::vector<bytes> &out, const void *in, size_t l);
    virtual size_t block_size();
};

template<class EncryptorCore>
class BlockEncryptor : Encryptor {
    EncryptorCore core;

    void load_state(DataLoader &d) {
        core.load_state(d);
    }

    void store_state(DataLoader &d) {
        core.store_state(d);
    }

    virtual void enc(std::vector<bytes> &out, const void *in, size_t l) {
        EncryptorCore::block_t *_out = out.insert(out.end(), 0, );
        return core.enc((EncryptorCore::block_t*)out, (const EncryptorCore::block_t*)in, l);
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

