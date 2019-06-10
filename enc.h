#ifndef __END_H_
#define __END_H_

class Encryptor {
    virtual void load_state(DataLoader &d);
    virtual void store_state(DataStorer &d);
    virtual void enc(void *out, const void *in, size_t l);
};

#endif /* __END_H_ */

