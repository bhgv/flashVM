/* Copyright (c) 2026 bhgv
 * EN: Private semantic controls suppress one copied phase without changing execution.
 * RU: Частные semantic controls скрывают одну copied phase, не меняя execution.
 */
#include "managed_workers.h"

extern int __real_dvm_resident_observe(unsigned long after,
                                       DVM_ResidentCard *cards,
                                       unsigned int capacity,
                                       unsigned int *count,
                                       int *more); /* Matched production reader. */


/* EN: Delegate original bounded copy for borrowed native outputs, then omit only
 * WAIT_WORK (CONTROL=1), DISPATCH (2) or channel ALT activity (3). No lifetime change.
 * RU: Делегировать bounded copy для borrowed native outputs, затем убрать только
 * WAIT_WORK (CONTROL=1), DISPATCH (2) либо ALT activity (3). Lifetime не меняется. */
int __wrap_dvm_resident_observe(
    unsigned long after, DVM_ResidentCard *cards, unsigned int capacity, unsigned int *count, int *more)
{
    unsigned int index;
    int result = __real_dvm_resident_observe(after, cards, capacity, count, more);
    if (result)
    {
        for (index = 0; index < *count; index++)
        {
            if ((CONTROL == 1 && cards[index].phase == DVM_RESIDENT_WAIT_WORK) ||
                (CONTROL == 2 && cards[index].phase == DVM_RESIDENT_DISPATCH))
            {
                cards[index].phase = DVM_RESIDENT_SCHEDULER;
            }
            if (CONTROL == 3 && cards[index].activity == DVM_ACTIVITY_ALT)
            {
                cards[index].activity = DVM_ACTIVITY_READY;
            }
        }
    }
    return result;
}
