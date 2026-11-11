/* Copyright (c) 2026 bhgv
 *
 * EN: Fast GC callbacks for shared retirement and retained releases.
 * RU: Проходы Fast GC для параллельного и резервного освобождения.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "../collector_passes.h"
#include "../../runtime_rc.h"
#include "../../managed_owner.h"
#include "../../exception_host.h"
#include <stdio.h>
#include <stdlib.h>


/* [GC:] EN: Run shared Fast work on context within budget. user is unused.
 * RU: Выполнить параллельный проход Fast на context с бюджетом budget. user не используется. */
unsigned int dvm_gc_fast_shared(DVM_NativeContext *context, size_t budget, void *user)
{
    (void)user;
    size_t reserve_budget = budget / 2 + budget % 2;
    unsigned int reserves = dvm_runtime_exception_retire_shared(context, reserve_budget);
    return reserves + dvm_runtime_rc_shared_leaves(context, budget - (reserves ? reserve_budget : 0));
}


/* [GC:] EN: Test for pending Fast work. user is unused.
 * RU: Проверить наличие заданий Fast. user не используется. */
int dvm_gc_fast_ready(void *user)
{
    (void)user;
    return dvm_runtime_rc_ready() || dvm_managed_owner_pending() != 0 || dvm_runtime_exception_retirement_ready();
}


/* [GC:] EN: Release up to budget owners with the world stopped. context is bound, user is unused.
 * RU: Освободить до budget ссылок при остановленной куче. context привязан, user не используется. */
DVM_GCWorkResult dvm_gc_fast_retained(DVM_NativeContext *context, size_t budget, void *user)
{
    unsigned int remaining;
    unsigned int tickets;
    USED(context);
    USED(user);
    dvm_gc_collector_environment();
    if (waserror())
    {
        fprintf(stderr, "disvm RC collector: %s\n", up->env->errstr);
        exit(1);
    }
    if (budget && dvm_runtime_exception_retirement_ready())
    {
        dvm_runtime_exception_retire_one();
        budget--;
    }
    tickets = dvm_managed_owner_pending() ? (unsigned int)(budget / 2) : 0;
    if (!tickets && dvm_managed_owner_pending())
    {
        tickets = (unsigned int)budget;
    }
    remaining = dvm_runtime_rc_drain((unsigned int)budget - tickets);
    remaining |= dvm_managed_owner_drain(tickets);
    remaining |= dvm_runtime_exception_retirement_ready();
    poperror();
    return remaining ? DVM_GC_MORE : DVM_GC_DONE;
}
