#pragma once

#include <cstdint>
#include <string>

#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/ec.h>
#include <openssl/objects.h>
#include <openssl/err.h>
#include <vector>

#include "../common.h"


typedef GuardPointer<EVP_MD_CTX, EVP_MD_CTX_free> EVP_MD_CTX_ptr;
typedef GuardPointer<EVP_PKEY, EVP_PKEY_free> EVP_PKEY_ptr;
typedef GuardPointer<EVP_PKEY_CTX, EVP_PKEY_CTX_free> EVP_PKEY_CTX_ptr;


class LibreSSLSigner {
    EVP_MD_CTX_ptr ctx = NULL;

    public:
    LibreSSLSigner(int type, const uint8_t *priv_key, size_t priv_key_len);
    size_t sign(const uint8_t *data, size_t data_len, uint8_t *out_buf, size_t out_len);
};

class LibreSSLVerifier {
    EVP_MD_CTX_ptr ctx = NULL;

    public:
    LibreSSLVerifier(int type, const uint8_t *pub_key, size_t pub_key_len);
    bool verify(const uint8_t *dgst, size_t dgst_len, const uint8_t *sgn, size_t sgn_len);
};

void generate_rsa_key(size_t bits, std::vector<uint8_t> &pubkey, std::vector<uint8_t> &privkey);
void generate_ecdsa_key(int curve_nid, std::vector<uint8_t> &pubkey, std::vector<uint8_t> &privkey);
