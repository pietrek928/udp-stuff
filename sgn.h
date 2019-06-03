#ifndef __IDENTITY_H_
#define __IDENTITY_H_

#include "err.h"

class Identity {
public:
    virtual void verify(const void *sgn, const void *dgst, size_t len) {
        throw IdentityError(
            std::string("Verify not implemented for class ") + typeid(*this).name()
        );
    }

    virtual bool sign(const void *dgst, size_t len, void *out) {
        throw IdentityError(
            std::string("Verify not implemented for class ") + typeid(*this).name()
        );
    }
};

class ECDSAIdentity : public Identity {
    EC_KEY key;

    public:

    void verify(const void *sgn, const void *dgst, size_t len) {
        if (ECDSA_verify(, dgst, len, sgn, len, key)) {
            throw IdentityError(
                std::string("Signature verification failed")
            );
        }
    }
};

template<const int key_size>
class ECDSAIdentity : public ECDSAIdentity<key_size> {
    virtual bool sign(const void *dgst, size_t len, void *out) {
        ECDSA_sign(, dgst, len, out, &len, key);
    }
};

#endif /* __IDENTITY_H_ */

