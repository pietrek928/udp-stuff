#ifndef __ERR_H_
#define __ERR_H_

#include <string.h>
#include <string>
#include <exception>

#define __STR(x) #x
#define STR(x) __STR(x)
#define __AT__ __FILE__ ":" STR(__LINE__)

class SysError : public std::exception {
    std::string descr;

public:
    SysError(std::string descr) : descr(descr) {}

    const char *what() const throw () {
        return descr.c_str();
    }
};

void _scall(const char *descr, const char * cmd, int ret_val) {
    if (ret_val) {
        throw SysError(((std::string)cmd) + ": " + descr + " returned " + std::to_string(ret_val) + ", errno: " + strerror(errno));
    }
}

void _scall(const char *descr, const char *cmd, bool success) {
    if (!success) {
        throw SysError(((std::string)cmd) + ": " + descr + " failed, errno: " + strerror(errno));
    }
}

#define scall(descr, cmd) _scall(descr, __AT__ " " #cmd, cmd)

#endif /* __ERR_H_ */

