#include "log.h"

int logfd = -1;

int log_get_fd()
{
    return logfd;
}

void log_set_fd(int fd)
{
    logfd = fd;
}