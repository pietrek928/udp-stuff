#ifndef __IDENTITY_H_
#define __IDENTITY_H_

#include <openssl/ec.h>

#include "err.h"

typedef uint8_t byte;

class Identity {
public:
    virtual void verify(const void *sgn, const void *dgst, size_t len) {
        throw IdentityError(
            std::string("Verify not implemented for class ") + typeid(*this).name()
        );
    }

    void sign(const void *dgst, size_t len, void *out) {
        throw IdentityError(
            std::string("Verify not implemented for class ") + typeid(*this).name()
        );
    }

    virtual void init(Config *cfg) {
        throw IdentityError(
            std::string("Init not implemented for class ") + typeid(*this).name()
        );
    }
};

class ECDSAIdentity : public Identity {
    public:

    EC_KEY *key = NULL;

    void verify(const byte *sgn, const byte *dgst, size_t len) {
        if (ECDSA_verify(0, dgst, len, sgn, len, key)) {
            throw IdentityError(
                std::string("Signature verification failed")
            );
        }
    }

    ~ECDSAIdentity() {
        EC_KEY_free(key);
    }
};

class ECDSAPrivateIdentity : public ECDSAIdentity {
    void sign(const byte *dgst, size_t dgst_len, std::vector<byte> &out) {
        std::unique_ptr<byte, decltype(&::ECDSA_SIG_free)> sgn(
            ECDSA_do_sign(dgst, dgst_len, key)
        );
        pcall(
            "Signing message",
            sgn.get()
        );
        auto sig_len = ECDSA_size(sgn_len);
        out.insert(out.back(), sgn, sgn + sig_len);
        //ECDSA_SIG_free(sgn);
    }

    void init(Config *cfg) {
        const char *crv_name;
        CONFIGURE(crv);

        int nid;
        fcall(
            "Parsing curve name",
            (nid = OBJ_txt2nid()) != NID_undef
        );

        pcall(
            "Generating new key",
            key = EC_KEY_new_by_curve_name(nid)
        );
    }
};

#endif /* __IDENTITY_H_ */
