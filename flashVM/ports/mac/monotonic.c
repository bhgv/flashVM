/* FlashVM: macOS absolute-clock milliseconds for elapsed-time services. */
#include "../monotonic_port.h"
#include "../time_constants.h"
#include <mach/mach_time.h>
/* Apply the platform timebase using floating arithmetic to avoid integer product overflow. */
int fvm_port_monotonic_ms(double *output)
{
    mach_timebase_info_data_t scale;
    if (!output || mach_timebase_info(&scale) != KERN_SUCCESS || !scale.denom)
        return 0;
    *output = (double)mach_absolute_time() * (double)scale.numer / (double)scale.denom /
              FVM_TIME_NANOSECONDS_PER_MILLISECOND;
    return 1;
}
