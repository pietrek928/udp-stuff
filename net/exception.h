#pragma once

#include <stdexcept>

class ConnectionError : public std::exception {
    std::string descr;
    public:
    ConnectionError(std::string descr) : descr(descr) {}
};