/* Copyright (c) 2026 bhgv
 * EN: Query actual resident metadata from concurrent real Sys callers.
 * RU: Читать actual resident metadata из concurrent real Sys callers.
 */
#include "managed_workers.h"
#include "resident_threads/observation_counter.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

static int observed;                         /* Atomic once-only diagnostic publication. */
extern void __real_Sys_werrstr(void *frame); /* Matched production builtin. */


/* EN/RU: Borrow rooted frame; verify copied pages, then delegate original Sys result.
 * Borrow rooted frame; проверить copied pages, затем вызвать original Sys result. */
void __wrap_Sys_werrstr(void *frame)
{
    DVM_ResidentCard cards[2];
    unsigned long boundary = (unsigned long)LLONG_MAX - 1; /* Private owner counter. */
    increment_resident_counter(&boundary);
    increment_resident_counter(&boundary);
    if (boundary != (unsigned long)LLONG_MAX)
    {
        fprintf(stderr, "FAIL resident counter saturation\n");
        exit(14);
    }
    unsigned int count = 99;
    unsigned int index;
    unsigned long cursor = 0; /* Copied ID only, never a resident pointer. */
    int more = 99;
    if (!dvm_resident_observe(0, cards, 2, &count, &more) || count > 2 || (more != 0 && more != 1))
    {
        fprintf(stderr, "FAIL live registry page\n");
        exit(14);
    }
    for (index = 0; index < count; index++)
    {
        if (!cards[index].id || cards[index].pid <= 0 || (cards[index].done != 0 && cards[index].done != 1) ||
            cards[index].phase < DVM_RESIDENT_STARTING || cards[index].phase > DVM_RESIDENT_DONE ||
            (cards[index].done != (cards[index].phase == DVM_RESIDENT_DONE)) ||
            cards[index].activity < DVM_ACTIVITY_UNKNOWN || cards[index].activity > DVM_ACTIVITY_BROKEN ||
            cards[index].turns > cards[index].dispatches || cards[index].dispatches > (unsigned long)LLONG_MAX ||
            cards[index].waits > (unsigned long)LLONG_MAX || (index && cards[index - 1].id >= cards[index].id))
        {
            fprintf(stderr, "FAIL live registry identity\n");
            exit(14);
        }
    }
    if (count && !__atomic_exchange_n(&observed, 1, __ATOMIC_RELAXED))
    {
        fprintf(stderr, "NATIVELIVE_REGISTRY observed=1 bounded=1\n");
    }
    if (count)
    {
        cursor = cards[count - 1].id;
        if (!dvm_resident_observe(cursor, cards, 2, &count, &more))
        {
            exit(14);
        }
        for (index = 0; index < count; index++)
        {
            if (cards[index].id <= cursor || (index && cards[index - 1].id >= cards[index].id))
            {
                fprintf(stderr, "FAIL live registry continuation cursor\n");
                exit(14);
            }
        }
    }
    if (!dvm_resident_observe(ULONG_MAX, cards, 2, &count, &more) || count || more)
    {
        fprintf(stderr, "FAIL live registry end cursor\n");
        exit(14);
    }
    count = 99;
    more = 99;
    cards[0].id = 1234;
    if (dvm_resident_observe(0, cards, 0, &count, &more) || dvm_resident_observe(0, cards, 257, &count, &more) ||
        dvm_resident_observe(0, NULL, 2, &count, &more) || dvm_resident_observe(0, cards, 2, NULL, &more) ||
        dvm_resident_observe(0, cards, 2, &count, NULL) || count != 99 || more != 99 || cards[0].id != 1234)
    {
        fprintf(stderr, "FAIL live registry refusal publication\n");
        exit(14);
    }
    __real_Sys_werrstr(frame);
}
