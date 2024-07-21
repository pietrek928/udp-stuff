#include "ssl.h"

#include "call_check.h"

void SSLInit() {
    scall("initializing ssl library", SSL_library_init());
    scall("loading error strings", SSL_load_error_strings());
    scall("adding openssl algorithms", OpenSSL_add_all_algorithms());
}

SSL_CTX_ptr SSLCreateContext() {
    SSL_CTX_ptr ctx(SSL_CTX_new(TLS_method()));
    scall("creating ssl context", ctx);

    // level 4 = 256-bit security
    SSL_CTX_set_security_level(ctx, 4);

    scall(
        "ssl ctx - setting minimum protocol version",
        SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION)
    );

    const char *cipher_list = (  // TODO: verify ciphers
        "ECDHE-ECDSA-AES256-GCM-SHA384:"
        "ECDHE-RSA-AES256-GCM-SHA384:"
        "ECDHE-ECDSA-CHACHA20-POLY1305:"
        "ECDHE-RSA-CHACHA20-POLY1305:"
        "ECDHE-ECDSA-AES128-GCM-SHA256:"
        "ECDHE-RSA-AES128-GCM-SHA256:"
        "ECDHE-ECDSA-AES256-SHA384:"
        "ECDHE-RSA-AES256-SHA384"
    );
    scall(
        "ssl ctx - setting cipher list",
        SSL_CTX_set_cipher_list(ctx, cipher_list)
    );

    // Disable compression to prevent CRIME attack
    SSL_CTX_set_options(ctx, SSL_OP_NO_COMPRESSION);

    // Enable OCSP stapling
    scall(
        "ssl ctx - enabling ocsp stapling",
        SSL_CTX_set_tlsext_status_type(ctx, TLSEXT_STATUSTYPE_ocsp)
    );

    // Set up session cache
    scall(
        "ssl ctx - setting session cache mode",
        SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_CLIENT)
    );
    scall(
        "ssl ctx - setting session id context",
        SSL_CTX_set_session_id_context(ctx, (const unsigned char*)"net_test", 5)
    );

    // // Optionally, set up certificate verification
    // SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
    // SSL_CTX_set_verify_depth(ctx, 4);
    // SSL_CTX_load_verify_locations(ctx, "/path/to/ca/file.pem", NULL);

    return ctx;
}

SSL_ptr SSLCreate(SSL_CTX *ctx, socket_t fd) {
    SSL_ptr ssl(SSL_new(ctx));
    scall("initializing ssl connection", ssl);
    scall("ssl connection - attaching socket", SSL_set_fd(ssl, fd));
    return ssl;
}
