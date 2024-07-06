#include "log.h"

#include <sys/time.h>
#include <iomanip>

const char *log_level_to_colored_name(LogLevel l) {
    switch(l) {
        case LogLevel::NOTSET: return "?";
        case LogLevel::DEBUG: return BASH_SET_COLOR(1, 90) "dbg" BASH_CLEAR_COLOR;
        case LogLevel::INFO: return BASH_SET_COLOR(1, 34) "info" BASH_CLEAR_COLOR;
        case LogLevel::WARNING: return BASH_SET_COLOR(1, 93) "warn" BASH_CLEAR_COLOR;
        case LogLevel::ERROR: return BASH_SET_COLOR(1, 31) "err" BASH_CLEAR_COLOR;
        case LogLevel::CRITICAL: return BASH_SET_COLOR(1, 91) "critical" BASH_CLEAR_COLOR;
        default: return "";
    }
}

std::ostream &start_log_line_stream(LogLevel level, const char *channel_name) {
    struct timeval tv;
        gettimeofday(&tv, NULL);
        struct tm *lt = localtime(&tv.tv_sec);
        char tm_s[24];
        strftime(tm_s, sizeof(tm_s), "%F %X", lt);
        return std::cerr << tm_s << "." << std::setfill('0') << std::setw(3) << tv.tv_usec/1000
            << " " << log_level_to_colored_name(level) << " " << channel_name << ":: ";
}
