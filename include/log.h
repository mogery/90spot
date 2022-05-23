#include <stdio.h>

#ifndef _log_H
#define _log_H

#define LOG_NORMAL_SEQ "\x1b[0m"
#define LOG_DEBUG_SEQ "\x1b[1;30m"
#define LOG_ERROR_SEQ "\x1b[1;31m"
#define LOG_WARN_SEQ "\x1b[1;33m"

#ifdef __cplusplus
extern "C" {
#endif

FILE* log_get_fd();
void log_set_fd(FILE* fd);

#ifdef __cplusplus
}
#endif

#define _log(seq, ...) {\
    printf(seq __VA_ARGS__);\
    if (log_get_fd() != NULL) {\
        fprintf(log_get_fd(), __VA_ARGS__);\
    }\
}

#define log_info(...) {\
    _log(LOG_NORMAL_SEQ, __VA_ARGS__);\
}

#define log_debug(...) {\
    _log(LOG_DEBUG_SEQ, __VA_ARGS__);\
}

#define log_error(...) {\
    _log(LOG_ERROR_SEQ, __VA_ARGS__);\
}

#define log_warn(...) {\
    _log(LOG_WARN_SEQ, __VA_ARGS__);\
}

#endif