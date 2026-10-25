/* FlashVM: Windows performance-counter milliseconds for elapsed-time services. */
#include "../monotonic_port.h"
#include "../time_constants.h"
#include <windows.h>


/* Convert the monotonic performance counter without requiring a newer GetTickCount64 API. */
int fvm_port_monotonic_ms(double *output)
{
    LARGE_INTEGER counter, frequency;
    if (!output || !QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0 ||
        !QueryPerformanceCounter(&counter) || counter.QuadPart < 0)
    {
        return 0;
    }
    *output = (double)counter.QuadPart / (double)frequency.QuadPart * FVM_TIME_MILLISECONDS_PER_SECOND;
    return 1;
}
