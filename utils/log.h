#pragma once

#include "common.h"

#include <iostream>

enum class LogLevel {
    NOTSET = 0,
    DEBUG = 1,
    INFO = 2,
    WARNING = 3,
    WARN = WARNING,
    ERROR = 4,
    CRITICAL = 5,
    FATAL = CRITICAL,
};

#define BASH_ESC_CHAR "\x1b"
#define BASH_SET_COLOR(a, b) BASH_ESC_CHAR "[" #a ";" #b "m"
#define BASH_CLEAR_COLOR BASH_ESC_CHAR "[0m"

const char *log_level_to_colored_name(LogLevel l);

class NopStream {
public:

    template<class T>
    inline const NopStream &operator<<(T v) const {
        return *this;
    }
};
constexpr auto NOP_STREAM = NopStream();

std::ostream &start_log_line_stream(LogLevel level, const char *channel_name);

#ifndef LOG_LEVEL
#define LOG_LEVEL DEBUG
#endif /* LOG_LEVEL */

#define DBG_ENDL "\n"
#define DBG_STREAM(channel) std::cerr << #channel << " :: "

#define LOG_ENDL "\n"
#define LOG(level, channel) ( \
    (LogLevel::level >= LogLevel::LOG_LEVEL && LogLevel::level >= CHANNEL_LOG_LEVEL_ ## channel) \
    ? start_log_line_stream(LogLevel::level, #channel) : NOP_STREAM \
)

#define DBG(channel) LOG(DEBUG, channel)
#define INFO(channel) LOG(INFO, channel)
#define WARN(channel) LOG(WARNING, channel)
#define ERR(channel) LOG(ERROR, channel)
#define CRIT(channel) LOG(CRITICAL, channel)
