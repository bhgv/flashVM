/* Monotonic native clock, isolated from VM frame layouts and GUI dependencies. */
#if !defined(_WIN32) && !defined(__APPLE__)
#define _POSIX_C_SOURCE 200809L
#endif
#include "disvm/clock_port.h"
#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach/mach_time.h>
#else
#include <time.h>
#endif


/* Copy one successful platform sample; never expose a partial/error value.
 * Epoch is deliberately unspecified. No wall-clock or CPU-time fallback. */
int dvm_host_clock_ms(uint64_t *milliseconds)
{
    uint64_t result;
    if (!milliseconds)
    {
        return 0;
    }
#if defined(_WIN32)
    result = (uint64_t)GetTickCount64();
#elif defined(__APPLE__)
    mach_timebase_info_data_t units;
    long double value;
    if (mach_timebase_info(&units) != KERN_SUCCESS || !units.denom)
    {
        return 0;
    }
    value = (long double)mach_absolute_time() * units.numer / units.denom / 1000000.0L;
    if (value < 0 || value >= (long double)UINT64_MAX)
    {
        return 0;
    }
    result = (uint64_t)value;
#else
    struct timespec sample;
    uint64_t fraction;
    if (clock_gettime(CLOCK_MONOTONIC, &sample) || sample.tv_sec < 0 ||
        (uint64_t)sample.tv_sec > UINT64_MAX / 1000U)
    {
        return 0;
    }
    fraction = (uint64_t)sample.tv_nsec / 1000000U;
    if ((uint64_t)sample.tv_sec > (UINT64_MAX - fraction) / 1000U)
    {
        return 0;
    }
    result = (uint64_t)sample.tv_sec * 1000U + fraction;
#endif
    *milliseconds = result;
    return 1;
}
