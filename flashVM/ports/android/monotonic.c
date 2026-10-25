/* FlashVM: android monotonic milliseconds for AVM1 elapsed-time services. */
/* Request the POSIX clock_gettime declarations in strict C89 mode. */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif
#include "../monotonic_port.h"
#include "../time_constants.h"
#include <time.h>


/* Read elapsed clock time without consulting the adjustable calendar clock. */
int fvm_port_monotonic_ms(double *output)
{
    struct timespec stamp;
    if (!output || clock_gettime(CLOCK_MONOTONIC, &stamp))
    {
        return 0;
    }
    *output = (double)stamp.tv_sec * FVM_TIME_MILLISECONDS_PER_SECOND +
              (double)stamp.tv_nsec / FVM_TIME_NANOSECONDS_PER_MILLISECOND;
    return 1;
}
