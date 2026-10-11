#include "../clock_port.h"
#include <windows.h>
int fvm_port_clock_ms(double *out)
{
    FILETIME time;
    if (!out)
        return 0;
    GetSystemTimeAsFileTime(&time);
    *out = ((double)time.dwHighDateTime * 4294967296.0 + time.dwLowDateTime) / 10000.0 -
           11644473600000.0;
    return 1;
}
