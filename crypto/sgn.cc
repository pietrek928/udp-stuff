#include "sgn.h"

#include "exception.h"

LibreSSLSigner::LibreSSLSigner(int type, const uint8_t *priv_key, size_t priv_key_len) {
    EVP_PKEY_ptr pkey = EVP_PKEY_new_raw_private_key(
        type, NULL, priv_key, priv_key_len
    );
    if (unlikely(!pkey)) {
        throw OSSLError("EVP_PKEY_new_raw_private_key");
    }

    ctx = EVP_MD_CTX_create();
    if (unlikely(!ctx)) {
        throw OSSLError("EVP_MD_CTX_create");
    }

    auto init_result = EVP_DigestSignInit(ctx, NULL, EVP_sha256(), NULL, pkey);
    if (unlikely(init_result != 1)) {
        throw OSSLError("EVP_DigestSignInit");
    }
}

size_t LibreSSLSigner::sign(const uint8_t *data, size_t data_len, uint8_t *out_buf, size_t out_len) {
    size_t sgn_len = out_len;
    auto result = EVP_DigestSign(
        ctx, out_buf, &sgn_len, data, data_len
    );
    if (unlikely(result != 1)) {
        throw OSSLError("EVP_DigestSign");
    }
    if (unlikely(sgn_len > out_len)) {
        throw OSSLError("EVP_DigestSign: buffer too small", -1);
    }
    return sgn_len;
}


LibreSSLVerifier::LibreSSLVerifier(int type, const uint8_t *pub_key, size_t pub_key_len) {
    EVP_PKEY_ptr pkey = EVP_PKEY_new_raw_public_key(
        type, NULL, pub_key, pub_key_len
    );
    if (unlikely(!pkey)) {
        throw OSSLError("EVP_PKEY_new_raw_public_key");
    }

    ctx = EVP_MD_CTX_create();
    if (unlikely(!ctx)) {
        throw OSSLError("EVP_MD_CTX_create");
    }

    auto init_result = EVP_DigestVerifyInit(ctx, NULL, EVP_sha256(), NULL, pkey);
    if (unlikely(init_result != 1)) {
        throw OSSLError("EVP_DigestVerifyInit");
    }
}

bool LibreSSLVerifier::verify(const uint8_t *dgst, size_t dgst_len, const uint8_t *sgn, size_t sgn_len) {
    auto result = EVP_DigestVerify(
        ctx, sgn, sgn_len, dgst, dgst_len
    );
    if (unlikely(result == -1)) {
        throw OSSLError("EVP_DigestVerify");
    }
    return result == 1;
}


void generate_rsa_key(
    size_t bits, std::vector<uint8_t> &pubkey, std::vector<uint8_t> &privkey
) {
    EVP_PKEY_CTX_ptr pkey = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    if (unlikely(!pkey)) {
        throw OSSLError("EVP_PKEY_CTX_new_id");
    }

    auto init_result = EVP_PKEY_keygen_init(pkey);
    if (unlikely(init_result != 1)) {
        throw OSSLError("EVP_PKEY_keygen_init");
    }

    auto set_bits_result = EVP_PKEY_CTX_set_rsa_keygen_bits(pkey, bits);
    if (unlikely(set_bits_result != 1)) {
        throw OSSLError("EVP_PKEY_CTX_set_rsa_keygen_bits");
    }

    GuardPointer<EVP_PKEY, EVP_PKEY_free> key = NULL;
    auto gen_result = EVP_PKEY_keygen(pkey, &key.ptr);
    if (unlikely(gen_result != 1)) {
        throw OSSLError("EVP_PKEY_keygen");
    }

    size_t pubkey_len = 0;
    auto get_pubkey_len_result = EVP_PKEY_get_raw_public_key(key, NULL, &pubkey_len);
    if (unlikely(get_pubkey_len_result != 1)) {
        throw OSSLError("EVP_PKEY_get_raw_public_key");
    }
    pubkey.resize(pubkey_len);
    auto get_pubkey_result = EVP_PKEY_get_raw_public_key(key, pubkey.data(), &pubkey_len);
    if (unlikely(get_pubkey_result != 1)) {
        throw OSSLError("EVP_PKEY_get_raw_public_key");
    }

    size_t privkey_len = 0;
    auto get_privkey_len_result = EVP_PKEY_get_raw_private_key(key, NULL, &privkey_len);
    if (unlikely(get_privkey_len_result != 1)) {
        throw OSSLError("EVP_PKEY_get_raw_private_key");
    }
    privkey.resize(privkey_len);
    auto get_privkey_result = EVP_PKEY_get_raw_private_key(key, privkey.data(), &privkey_len);
    if (unlikely(get_privkey_result != 1)) {
        throw OSSLError("EVP_PKEY_get_raw_private_key");
    }
}

void generate_ecdsa_key(int curve_nid, std::vector<uint8_t> &pubkey, std::vector<uint8_t> &privkey) {
    EVP_PKEY_CTX_ptr pkey = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
    if (unlikely(!pkey)) {
        throw OSSLError("EVP_PKEY_CTX_new_id");
    }

    auto init_result = EVP_PKEY_keygen_init(pkey);
    if (unlikely(init_result != 1)) {
        throw OSSLError("EVP_PKEY_keygen_init");
    }

    auto set_bits_result = EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pkey, NID_X9_62_prime256v1);
    if (unlikely(set_bits_result != 1)) {
        throw OSSLError("EVP_PKEY_CTX_set_ec_paramgen_curve_nid");
    }

    GuardPointer<EVP_PKEY, EVP_PKEY_free> key = NULL;
    auto gen_result = EVP_PKEY_keygen(pkey, &key.ptr);
    if (unlikely(gen_result != 1)) {
        throw OSSLError("EVP_PKEY_keygen");
    }

    size_t pubkey_len = 0;
    auto get_pubkey_len_result = EVP_PKEY_get_raw_public_key(key, NULL, &pubkey_len);
    if (unlikely(get_pubkey_len_result != 1)) {
        throw OSSLError("EVP_PKEY_get_raw_public_key");
    }
    pubkey.resize(pubkey_len);
    auto get_pubkey_result = EVP_PKEY_get_raw_public_key(key, pubkey.data(), &pubkey_len);
    if (unlikely(get_pubkey_result != 1)) {
        throw OSSLError("EVP_PKEY_get_raw_public_key");
    }

    size_t privkey_len = 0;
    auto get_privkey_len_result = EVP_PKEY_get_raw_private_key(key, NULL, &privkey_len);
    if (unlikely(get_privkey_len_result != 1)) {
        throw OSSLError("EVP_PKEY_get_raw_private_key");
    }
    privkey.resize(privkey_len);
    auto get_privkey_result = EVP_PKEY_get_raw_private_key(key, privkey.data(), &privkey_len);
    if (unlikely(get_privkey_result != 1)) {
        throw OSSLError("EVP_PKEY_get_raw_private_key");
    }
}
