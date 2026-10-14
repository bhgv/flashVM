/* FlashVM: validated monotonic clock reads shared by context creation and elapsed-time services. */
#ifndef FVM_TIMER_HPP
#define FVM_TIMER_HPP /* Monotonic sample validation guard. */
#include "../../ports/monotonic_port.h"
#include <limits>
/* Publish only successful, finite, nonnegative clock samples. */
inline bool fvm_timer_read(double *output)
{
    double sample = 0;
    if (!fvm_port_monotonic_ms(&sample) || !(sample >= 0) ||
        sample > std::numeric_limits<double>::max())
        return false;
    *output = sample;
    return true;
}
#endif
