#ifndef ERR_STUB_H
#define ERR_STUB_H

/*
Stub err.h because the original is not in the reduced libc used by STM32
*/

#include <stdarg.h>
#include <stdio.h>

static inline void warnx(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    if (fmt) {
        vfprintf(stderr, fmt, args);
    }
    fprintf(stderr, "\n");
    va_end(args);
}

#endif