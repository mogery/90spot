#include "log.h"

FILE* logfd = NULL;

FILE* log_get_fd()
{
    return logfd;
}

void log_set_fd(FILE* fd)
{
    logfd = fd;
}