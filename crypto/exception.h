#pragma once

#include <string>
#include <exception>

#include <openssl/err.h>

class IdentityError : public std::exception {
    std::string descr;

public:
    IdentityError(std::string descr) : descr(descr) {}

    const char *what() const throw () {
        return descr.c_str();
    }
};

class IntegrityError : public std::exception {
    std::string descr;

public:
    IntegrityError(std::string descr) : descr(descr) {}

    const char *what() const throw () {
        return descr.c_str();
    }
};

class OSSLError : public std::exception {
    const char *descr;
    long err;

public:
    OSSLError(const char * descr, long err) : descr(descr), err(err) {}
    OSSLError(const char * descr) : descr(descr) {
        err = ERR_get_error();
    }

    const char *what() const throw () {
        return descr;
    }

    std::string describe() {
        char buf[256];
        ERR_error_string_n(err, buf, sizeof(buf));
        return std::string(descr) + ": " +  std::string(buf);
    }
};
