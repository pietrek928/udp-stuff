#ifndef __IDENTITY_H_
#define __IDENTITY_H_

#include <openssl/ec.h>
#include <openssl/objects.h>
#include <openssl/err.h>

#include "common.h"
#include "err.h"


class Identity {
public:
    virtual void verify(const byte_t *dgst, size_t dgst_len, const byte_t *sgn, size_t sgn_len) {
        throw IdentityError(
            std::string("Verify not implemented for class ") + typeid(*this).name()
        );
    }

    virtual void sign(const byte_t *dgst, size_t dgst_len, std::vector<byte_t> &out) {
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

    void verify(const byte_t *dgst, size_t dgst_len, const byte_t *sgn, size_t sgn_len) {
        auto r = ECDSA_verify(0, dgst, dgst_len, sgn, sgn_len, key);
        if (UNLIKELY(r != 1)) {
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
    public:

    void sign(const byte_t *dgst, size_t dgst_len, std::vector<byte_t> &out) {
        unsigned int sig_len = ECDSA_size(key);
        auto new_sig_len = sig_len;

        auto inserted_pos = out.insert(out.end(), sig_len, 0);
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

        const char *crv;
        CONFIGURE(crv);
        crv = "secp256k1";

        int nid;
        fcall(
            "Parsing curve name",
            (nid = OBJ_txt2nid(crv)) != NID_undef
        );

        pcall(
            "Initializing key",
            key = EC_KEY_new_by_curve_name(nid)
        );

        fcall(
            "Generating EC key",
            EC_KEY_generate_key(key)
        );
    }
};

#endif /* __IDENTITY_H_ */

