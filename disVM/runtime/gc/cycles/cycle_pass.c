/* Copyright (c) 2026 bhgv
 *
 * EN: Shared candidate callback and retained cycle tracing.
 * RU: Обход очереди кандидатов и резервная трассировка циклов.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "../collector_passes.h"
#include "../../runtime_rc.h"
#include "../../runtime_gc.h"
#include <stdio.h>
#include <stdlib.h>


extern ulong gcepochs; /* Completed retained trace epochs. */


/* EN: Walk shared candidates on context within budget. user is unused.
 * RU: Обойти кандидатов на context в пределах budget. user не используется. */
unsigned int dvm_gc_cycles_shared(DVM_NativeContext *context, size_t budget, void *user)
{
    (void)user;
    return dvm_runtime_rc_shared_candidates(context, budget);
}


/* EN: Trace the registered process roots and update idle_passes, idle_epoch and passes for the next pass.
 * RU: Обойти roots и обновить счётчики idle_passes, idle_epoch и passes. */
DVM_GCWorkResult dvm_gc_cycles_trace(unsigned int *idle_passes,
    unsigned long *idle_epoch, unsigned long *passes)
{
    dvm_gc_collector_environment();
    if (waserror())
    {
        fprintf(stderr,"disvm cycle collector: %s\n",up->env->errstr);
        exit(1);
    }
    if (dvm_runtime_idle_cycles && !(*idle_passes))
    {
        (*idle_epoch) = gcepochs;
    }
    rungc(dvm_runtime_process_roots());
    poperror();
    (*passes)++;
    /* A native wait can leave the heap idle for many passes without another
     * dispatch request. Continue across the color rotation, with a hard cap. */
    if (dvm_runtime_idle_cycles && !gchalt &&
        ++(*idle_passes) < 256 && gcepochs - (*idle_epoch) < 3)
    {
        return DVM_GC_MORE;
    }
    (*idle_passes) = 0;
    return DVM_GC_DONE;
}
