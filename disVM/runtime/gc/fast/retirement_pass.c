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
#include "service_budget.inc"


/* [GC:] EN: Run shared Fast on bound context within budget; user is unused.
 * Nonempty ticket/reserve/RC services receive independent allowances with a
 * rotating remainder. Return progress/retry; never spend another service's
 * allowance on a blocked reader. Zero budget/unbound/foreign context returns
 * zero before scheduling. Ownership stays with each service on refusal.
 * RU: Выполнить shared Fast на привязанном context с budget; user не нужен.
 * Непустые ticket/reserve/RC получают независимые квоты с круговым остатком.
 * Вернуть работу/повтор; ожидание читателя не расходует чужую квоту. При отказе
 * владельцы остаются в своих службах. Нулевой budget/чужой или непривязанный
 * context дают ноль до планирования. */
unsigned int dvm_gc_fast_shared(DVM_NativeContext *context, size_t budget, void *user)
{
    (void)user;
    if (!budget || !context || context != dvm_native_context_current())
    {
        return 0;
    }
    size_t shares[FastServiceCount];
    fast_service_budget(budget, shares);
    unsigned int tickets = shares[FastTickets] ? dvm_managed_owner_drain_shared(context, shares[FastTickets]) : 0;
    unsigned int reserves =
        shares[FastReserves] ? dvm_runtime_exception_retire_shared(context, shares[FastReserves]) : 0;
    unsigned int owners = shares[FastRC] ? dvm_runtime_rc_shared_leaves(context, shares[FastRC]) : 0;
    return tickets + reserves + owners;
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
