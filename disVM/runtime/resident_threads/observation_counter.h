/* Copyright (c) 2026 bhgv
 * EN: Single-writer native diagnostic counters with signed-big saturation.
 * RU: Native diagnostic counters с одним writer и насыщением до signed-big max.
 */
#ifndef DVM_RESIDENT_OBSERVATION_COUNTER_H
#define DVM_RESIDENT_OBSERVATION_COUNTER_H
#include <limits.h>


/* EN: Increment borrowed native counter owned by this thread, saturating at
 * signed-big max. Release orders dispatch before its completed turn. No lock,
 * allocation, target access or failure; never use this for multiple writers.
 * RU: Увеличить borrowed native counter своего потока до signed-big max. Release
 * упорядочивает dispatch перед завершённым turn. Без lock/allocation/target access;
 * не применять к counter с несколькими writers. */
static void increment_resident_counter(unsigned long *counter)
{
    unsigned long value = __atomic_load_n(counter, __ATOMIC_RELAXED);
    if (value < (unsigned long)LLONG_MAX)
    {
        __atomic_store_n(counter, value + 1, __ATOMIC_RELEASE);
    }
}
#endif
