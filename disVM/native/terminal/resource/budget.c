/* Copyright (c) 2026 bhgv
 * EN: Shared monotonic resource-exchange budget, independent of caller and native worker role.
 * RU: Общий monotonic budget resource exchange независимо от роли caller/native worker.
 */
#define _POSIX_C_SOURCE 200809L
#include "internal.h"


/* EN/RU: See internal.h; subtract before conversion so the configured finite budget cannot overflow.
 * См. internal.h; вычитание до преобразования исключает overflow ограниченного budget.
 */
unsigned movie_budget(const struct timespec *deadline)
{
    struct timespec now; /* Current monotonic native clock. */
    if (clock_gettime(CLOCK_MONOTONIC, &now))
    {
        return 0;
    }
    long seconds = (long)(deadline->tv_sec - now.tv_sec); /* At most configured 120 seconds while live. */
    long nanos = deadline->tv_nsec - now.tv_nsec; /* Normalize before rounding. */
    if (nanos < 0)
    {
        seconds--;
        nanos += 1000000000L;
    }
    if (seconds < 0 || (!seconds && !nanos))
    {
        return 0;
    }
    if (seconds >= 120)
    {
        return 120000;
    }
    return (unsigned)seconds * 1000 + (unsigned)((nanos + 999999L) / 1000000L);
}
