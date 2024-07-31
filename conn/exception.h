#pragma once

#include <stdexcept>

class ProtocolError : public std::exception {
    std::string descr;
    public:
    ProtocolError(std::string descr) : descr(descr) {}
    const char *what() const noexcept override {
        return descr.c_str();
    }
};
