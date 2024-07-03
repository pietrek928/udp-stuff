#pragma once

#include <openssl/ssl.h>

#include "../common.h"
#include "exception.h"

typedef GuardPointer<SSL_CTX, SSL_CTX_free> SSL_CTX_ptr;
typedef GuardPointer<SSL, SSL_free> SSL_ptr;

void SSLInit();
