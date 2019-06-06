#ifndef __IDENTITY_H_
#define __IDENTITY_H_

#include <openssl/ec.h>
#include <openssl/objects.h>

#include "common.h"
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
        if (key) {
            EC_KEY_free(key);
        }
    }
};

class ECDSAPrivateIdentity : public ECDSAIdentity {
    void sign(const byte *dgst, size_t dgst_len, std::vector<byte> &out) {
        unsigned int sig_len = ECDSA_size(key);
        auto new_sig_len = sig_len;

        auto inserted_pos = out.insert(out.end(), 0, sig_len);
        ECDSA_sign(0, dgst, dgst_len, &* inserted_pos, &new_sig_len, key);

        if (UNLIKELY(new_sig_len < sig_len)) {
            out.erase(inserted_pos + new_sig_len, out.end());
            fcall("Signing message", new_sig_len);
        }
    }

    void init(Config *cfg) {
        if ( key ) {
            EC_KEY_free(key);
            key = NULL;
        }

        const char *crv_name;
        CONFIGURE(crv_name);

        int nid;
        fcall(
            "Parsing curve name",
            (nid = OBJ_txt2nid(crv_name)) != NID_undef
        );

        pcall(
            "Generating new key",
            key = EC_KEY_new_by_curve_name(nid)
        );
    }
};

#endif /* __IDENTITY_H_ */

