/* POSIX/Windows monotonic network clock without SDL, VM objects or process-wide state. */
#ifndef _WIN32
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#endif
#include "clock.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif
/* Unsigned arithmetic intentionally reduces samples modulo the same 32-bit period on every ABI. */
int fvm_net_clock(unsigned long *out)
{
    unsigned long value;
#ifndef _WIN32
    struct timespec sample;
#endif
    if (!out)
        return 0;
#ifdef _WIN32
    value = (unsigned long)GetTickCount();
#else
    if (clock_gettime(CLOCK_MONOTONIC, &sample))
        return 0;
    value = (unsigned long)sample.tv_sec * FVM_NET_CLOCK_MS +
            (unsigned long)sample.tv_nsec / FVM_NET_CLOCK_NS;
#endif
    *out = value & FVM_NET_CLOCK_MASK;
    return 1;
}
