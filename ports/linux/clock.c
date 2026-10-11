#include "../clock_port.h"
#include <sys/time.h>
int fvm_port_clock_ms(double *out)
{
    struct timeval time;
    if (!out || gettimeofday(&time, 0))
        return 0;
    *out = (double)time.tv_sec * 1000 + (double)time.tv_usec / 1000;
    return 1;
}
