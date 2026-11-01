#include "disvm/clock_port.h"
#include "thread_port.h"
#include <stdio.h>


/* Exercise the real monotonic OS clock and a real nonbusy host wait. Independent
 * sampling stays ordered; null refusal and milliseconds advancement are active. */
int main(void)
{
    uint64_t first;
    uint64_t previous;
    uint64_t now;
    unsigned int i;
    if (dvm_host_clock_ms(NULL) || !dvm_host_clock_ms(&first))
    {
        return 1;
    }
    previous = first;
    for (i = 0; i < 10000; ++i)
    {
        if (!dvm_host_clock_ms(&now) || now < previous)
        {
            fputs("monotonic sample failed\n",stderr);
            return 1;
        }
        previous = now;
    }
    fvm_port_thread_sleep_us(20000);
    if (!dvm_host_clock_ms(&now) || now <= first)
    {
        fputs("millisecond clock did not advance over a host wait\n",stderr);
        return 1;
    }
    puts("PASS real monotonic port: null refusal, 10000 ordered samples and elapsed wait");
    return 0;
}
