#include "auth.h"
#include "sgn.h"
#include "call_check.h"

#include <openssl/ssl.h>

constexpr static char PEER_ID_SEED_1[] = "834h804n82y387ynx2307ny8x70236n4786yn38923";
constexpr static char PEER_ID_SEED_2[] = "83n1-9348yc5n780345mn83485mt483mtg7tm58c48";
PeerId512_t peer_id_from_sha512(const byte_t *data, size_t data_len) {
    PeerId512_t peer_id;

    SSLHasher hasher("sha512");

    hasher.start();
    hasher.put((const byte_t *)PEER_ID_SEED_1, sizeof(PEER_ID_SEED_1));
    hasher.put(data, data_len);
    hasher.put((const byte_t *)PEER_ID_SEED_2, sizeof(PEER_ID_SEED_2));
    hasher.finish((byte_t*)peer_id.id);

    for (int i=0; i<(1<<14); i++) {
        hasher.compute_hash((byte_t*)peer_id.id, sizeof(peer_id), (byte_t*)peer_id.id);
    }

    return peer_id;
}

PeerId512_t peer_id_from_cert(X509 *cert) {
    EVP_PKEY_ptr pubkey;
    scall("cert - getting public key", pubkey = X509_get_pubkey(cert));

    SSL_DATA_ptr pubkey_subject_info;
    auto subject_len = i2d_PUBKEY(pubkey, (unsigned char **)&pubkey_subject_info.ptr);
    if (subject_len <= 0 || !pubkey_subject_info.ptr) {
        throw SSLError("cert - transforming to subject form failed");
    }

    return peer_id_from_sha512(pubkey_subject_info.ptr, subject_len);
}

// constexpr int SSL_PEER_ID_idx = 1243;
constexpr int CERT_MAX_DEPTH = 4;

// TODO: add logging to verification
int ssl_peer_id_verify_handler(int preverify_ok, X509_STORE_CTX *ctx) {
    if (!preverify_ok) {
        // TODO: log, analyze error safely
        return 0;
    }

    try {
        // X509 *err_cert = X509_STORE_CTX_get_current_cert(ctx);
        // int err = X509_STORE_CTX_get_error(ctx);

        // int err_depth = X509_STORE_CTX_get_error_depth(ctx);
        // if (err_depth > CERT_MAX_DEPTH) {
        //     X509_STORE_CTX_set_error(ctx, X509_V_ERR_CERT_CHAIN_TOO_LONG);
        //     return 0;
        // }

        STACK_OF(X509) *chain = X509_STORE_CTX_get0_chain(ctx);
        auto chain_length = sk_X509_num(chain);
        if (chain_length < 1 || chain_length > CERT_MAX_DEPTH) {
            // we want 1 cert here for id verification
            throw SSLError("Invalid certificate chain length");
        }

        X509 *peer_cert;
        scall("ssl - getting peer cert from chain", peer_cert = sk_X509_value(chain, 0));

        PeerId512_t current_peer_id = peer_id_from_cert(peer_cert);

        auto store_idx = SSL_get_ex_data_X509_STORE_CTX_idx();
        if (store_idx < 0) {
            throw SSLError("ssl - getting store id failed");
        }
        SSL *ssl;
        scall("ssl - getting SSL from context", ssl = (SSL*)X509_STORE_CTX_get_ex_data(ctx, store_idx));
        const PeerId512_t *expected_peer_data;
        scall("ssl - getting expected peer id", (void*)(
            expected_peer_data = (const PeerId512_t *)SSL_get_ex_data(ssl, 0)
        ));

        if (!(current_peer_id == *expected_peer_data)) {
            throw SSLError("ssl - invalid peer id passed during connecting");
        }

    } catch(...) {
        // TODO: log, analyze error safely
        return 0;
    }

    // We got here - looks fine
    return 1;
}

void ssl_add_verify_peer_id(SSL *ssl, const PeerId512_t *peer_id_ptr) {
    scall("ssl - adding peer_id ptr", SSL_set_ex_data(ssl, 0, (void*)peer_id_ptr));
    SSL_set_verify(
        ssl, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT | SSL_VERIFY_CLIENT_ONCE,
        ssl_peer_id_verify_handler
    );
}

void ssl_clear_verify_peer_id(SSL *ssl) {
    scall("ssl - reset peer_id data", SSL_set_ex_data(ssl, 0, NULL));
    SSL_set_verify(ssl, 0, NULL);
}
