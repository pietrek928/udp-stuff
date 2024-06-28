#pragma once

#include <string.h>
#include <string>
#include <exception>

#include "common.h"


class SysError : public std::exception {
    std::string descr;

public:
    SysError(std::string descr) : descr(descr) {}

    const char *what() const throw () {
        return descr.c_str();
    }
};

void __serr(const char *descr, const char * cmd, int ret_val) {
    throw SysError(((std::string)cmd) + ": " + descr + " returned " + std::to_string(ret_val) + ", errno: " + strerror(errno));
}

inline void _scall(const char *descr, const char *cmd, int ret_val) {
    if (UNLIKELY(ret_val)) {
        __serr(descr, cmd, ret_val);
    }
}

void __serr(const char *descr, const char *cmd) {
    throw SysError(((std::string)cmd) + ": " + descr + " failed, errno: " + strerror(errno));
}

inline void _scall(const char *descr, const char *cmd, bool success) {
    if (unlikely(!success)) {
        __serr(descr, cmd);
    }
}

#define scall(descr, cmd) _scall(descr, __AT__ " " #cmd, cmd)

void __perr(const char *descr, const char *cmd) {
    throw SysError(((std::string)cmd) + ": " + descr + " returned NULL");
}

inline void _pcall(const char *descr, const char *cmd, void *ptr) {
    if (unlikely(!ptr)) {
        __perr(descr, cmd);
    }
}

#define pcall(descr, cmd) _pcall(descr, __AT__ " " #cmd, cmd)

void __ferr(const char *descr, const char *cmd) {
    throw SysError(((std::string)cmd) + ": " + descr + " failed");
}

inline void _fcall(const char *descr, const char *cmd, bool cond) {
    if (unlikely(!cond)) {
        __ferr(descr, cmd);
    }
}

#define fcall(descr, cmd) _fcall(descr, __AT__ " " #cmd, cmd)
