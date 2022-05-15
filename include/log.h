#include <stdio.h>

#ifndef _log_H
#define _log_H

int log_get_fd();
void log_set_fd(int fd);

#define log(...) {\
    printf(__VA_ARGS__);\
    if (log_get_fd() != -1) {\
        dprintf(log_get_fd(), __VA_ARGS__);\
    }\
}

#endif