#include "sgn.h"

#include "call_check.h"


void SSLFreeData(unsigned char *data_ptr) {
    OPENSSL_free(data_ptr);
}

SSLSigner::SSLSigner(int type, const byte_t *priv_key, size_t priv_key_len) {
    EVP_PKEY_ptr pkey = EVP_PKEY_new_raw_private_key(
        type, NULL, priv_key, priv_key_len
    );
    scall("EVP_PKEY_new_raw_private_key", pkey);

    ctx = EVP_MD_CTX_create();
    scall("EVP_MD_CTX_create", ctx);

    scall(
        "signer - inituializing signing context",
        EVP_DigestSignInit(ctx, NULL, EVP_sha256(), NULL, pkey)
    );
}

size_t SSLSigner::sign(
    const byte_t *data, size_t data_len, byte_t *out_buf, size_t out_len
) {
    size_t sgn_len = out_len;
    scall("signer - signing digest", EVP_DigestSign(
        ctx, out_buf, &sgn_len, data, data_len
    ));
    if (unlikely(sgn_len > out_len)) {
        throw SSLError("EVP_DigestSign: buffer too small", -1);
    }
    return sgn_len;
}


SSLVerifier::SSLVerifier(int type, const byte_t *pub_key, size_t pub_key_len) {
    EVP_PKEY_ptr pkey = EVP_PKEY_new_raw_public_key(
        type, NULL, pub_key, pub_key_len
    );
    scall("EVP_PKEY_new_raw_public_key", pkey);

    ctx = EVP_MD_CTX_create();
    scall("EVP_MD_CTX_create", ctx);

    scall(
        "verifier - initializing verification context",
        EVP_DigestVerifyInit(ctx, NULL, EVP_sha256(), NULL, pkey)
    );
}

bool SSLVerifier::verify(
    const byte_t *dgst, size_t dgst_len, const byte_t *sgn, size_t sgn_len
) {
    auto result = EVP_DigestVerify(
        ctx, sgn, sgn_len, dgst, dgst_len
    );
    switch(result) {
        case 1:
            return true;
        case 0:
            return false;
        default:
            throw SSLError("EVP_DigestVerify");
    }
}


void generate_rsa_key(
    size_t bits, std::vector<byte_t> &pubkey, std::vector<byte_t> &privkey
) {
    EVP_PKEY_CTX_ptr pkey = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    scall("EVP_PKEY_CTX_new_id", pkey);

    scall("rsa keygen - init", EVP_PKEY_keygen_init(pkey));

    scall("rsa keygen - set bits", EVP_PKEY_CTX_set_rsa_keygen_bits(pkey, bits));

    GuardPointer<EVP_PKEY, EVP_PKEY_free> key;
    scall("rsa keygen", EVP_PKEY_keygen(pkey, &key.ptr));

    size_t pubkey_len = 0;
    scall(
        "rsa keygen - get pubkey len",
        EVP_PKEY_get_raw_public_key(key, NULL, &pubkey_len)
    );
    pubkey.resize(pubkey_len);
    scall(
        "rsa keygen - get pubkey",
        EVP_PKEY_get_raw_public_key(key, pubkey.data(), &pubkey_len)
    );

    size_t privkey_len = 0;
    scall(
        "rsa keygen - get privkey len",
        EVP_PKEY_get_raw_private_key(key, NULL, &privkey_len)
    );
    privkey.resize(privkey_len);
    scall(
        "rsa keygen - get privkey",
        EVP_PKEY_get_raw_private_key(key, privkey.data(), &privkey_len)
    );
}

void generate_ecdsa_key(
    int curve_nid, std::vector<byte_t> &pubkey, std::vector<byte_t> &privkey
) {
    EVP_PKEY_CTX_ptr pkey = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
    scall("EVP_PKEY_CTX_new_id", pkey);

    scall(
        "ecdsa keygen - init",
        EVP_PKEY_keygen_init(pkey)
    );

    scall(
        "ecdsa keygen - set curve",
        EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pkey, NID_X9_62_prime256v1)
    );

    GuardPointer<EVP_PKEY, EVP_PKEY_free> key;
    scall(
        "ecdca keygen",
        EVP_PKEY_keygen(pkey, &key.ptr)
    );

    size_t pubkey_len = 0;
    scall(
        "ecdsa keygen - get pubkey len",
        EVP_PKEY_get_raw_public_key(key, NULL, &pubkey_len)
    );
    pubkey.resize(pubkey_len);
    scall(
        "ecdsa keygen - get pubkey",
        EVP_PKEY_get_raw_public_key(key, pubkey.data(), &pubkey_len)
    );

    size_t privkey_len = 0;
    scall(
        "ecdsa keygen - get privkey len",
        EVP_PKEY_get_raw_private_key(key, NULL, &privkey_len)
    );
    privkey.resize(privkey_len);
    scall(
        "ecdsa keygen - get privkey",
        EVP_PKEY_get_raw_private_key(key, privkey.data(), &privkey_len)
    );
}

SSLHasher::SSLHasher(const char *hash_name) {
    const EVP_MD *md;
    scall("looking for hash", (void*)(
        md = EVP_get_digestbyname(hash_name)
    ));

    scall("init hasher ctx", ctx = EVP_MD_CTX_new());
}

void SSLHasher::start() {
    scall("hashing - init", EVP_DigestInit_ex2(ctx, md, NULL));
}
void SSLHasher::put(const byte_t *data, size_t data_len) {
    scall("hashing - append data", EVP_DigestUpdate(ctx, data, data_len));
}
size_t SSLHasher::finish(byte_t *out_data) {
    unsigned int s;
    scall("hashing - retrieving digest", EVP_DigestFinal_ex(ctx, (unsigned char*)out_data, &s));
    return s;
}
void SSLHasher::reset() {
    scall("hashing - resetting context", EVP_MD_CTX_reset(ctx));
}

size_t SSLHasher::compute_hash(const byte_t *data, size_t data_len, byte_t *out_data) {
    try {
        start();
        put(data, data_len);
        return finish(out_data);
    } catch(...) {
        reset();
        throw;
    }
};
