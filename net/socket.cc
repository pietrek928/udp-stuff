#include "socket.h"

#include "call_check.h"
#include <fcntl.h>
#include <sys/socket.h>

void set_socket_blocking(socket_t fd, bool blocking) {
    int flags = fcntl(fd, F_GETFL, 0);
    int old_flags = flags;
    ccall("getting flags", flags);
    if (blocking) {
        flags &= ~O_NONBLOCK;
    } else {
        flags |= O_NONBLOCK;
    }
    if (flags != old_flags) {
        ccall("setting flags", fcntl(fd, F_SETFL, flags));
    }
}

void set_socket_timeout(socket_t fd, float timeout_sec) {
    timeval tv;
    tv.tv_sec = (time_t)timeout_sec;
    tv.tv_usec = (long)((timeout_sec - tv.tv_sec) * 1e6);
    ccall("setting timeout", setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)));
    ccall("setting timeout", setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)));
}
