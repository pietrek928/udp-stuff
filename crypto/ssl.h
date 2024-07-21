#pragma once

#include "../utils/common.h"
#include "../net/socket.h"
#include "exception.h"

#include <openssl/ssl.h>

typedef GuardPointer<SSL_CTX, SSL_CTX_free> SSL_CTX_ptr;
typedef GuardPointer<SSL, SSL_free> SSL_ptr;

SSL_CTX_ptr SSLCreateContext();

void SSLInit();
SSL_ptr SSLCreate(SSL_CTX *ctx, socket_t fd);
