/* Copyright (c) 2026 bhgv
 * EN: Verify actual permanent owner and bounded diagnostic service; isolate omissions.
 * RU: Проверить actual permanent owner и bounded diagnostic service; отдельные omissions.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "managed_workers.h"
#include "disvm/native_context.h"
#include <stdio.h>
#include <stdlib.h>

static int observed; /* Atomic once-only owner-bound evidence. */
extern void __real_dvm_diagnostic_answer(DVM_DiagnosticMailbox *, const DVM_DiagnosticSample *, unsigned int);
extern int __real_dvm_diagnostic_pending(DVM_DiagnosticMailbox *);
extern void __real_dvm_diagnostic_forget_requester(DVM_DiagnosticMailbox *, int);


/* EN/RU: Fail only on exact semantic queue invariant, never via an assertion.
 * Ошибка только по точному semantic queue invariant, без assertion. */
static void unitcheck(int valid)
{
    if (!valid)
    {
        fprintf(stderr, "FAIL diagnostic native quota FIFO or close invariant\n");
        exit(14);
    }
}


/* EN: Read a synthetic native ticket; non-Ready must clear the entire output.
 * RU: Прочитать synthetic native ticket; non-Ready обнуляет весь output. */
static void unitreply(DVM_DiagnosticMailbox *box, unsigned long ticket, int status, int64_t cookie)
{
    DVM_DiagnosticSample sample = {99, 99, 99, 99, 99};
    int result = dvm_diagnostic_reply(box, 17, ticket, &sample);
    unitcheck(result == status);
    if (status == DVM_DIAG_READY)
    {
        unitcheck(sample.owner_pid == 23 && sample.cookie == cookie && sample.dispatches == 9 && sample.turns == 7 &&
                  sample.waits == 5);
    }
    else
    {
        unitcheck(!sample.owner_pid && !sample.cookie && !sample.dispatches && !sample.turns && !sample.waits);
    }
}


/* EN: Exercise the actual queue independently: completed-credit quota, oldest
 * pending FIFO after slot reuse, ownership, forgetting, close and zero outputs.
 * RU: Проверить actual queue: quota ready credits, FIFO после reuse slots,
 * ownership, forgetting, close и zero outputs. Private box is destroyed here. */
static void native_queue_unit(void)
{
    DVM_DiagnosticMailbox *box = dvm_diagnostic_create();
    DVM_DiagnosticSample sample = {23, 0, 9, 7, 5};
    DVM_DiagnosticSample output;
    unsigned long tickets[8];
    unsigned long later[2];
    unsigned long extra = 123;
    unsigned int index;
    unitcheck(box != NULL);
    unitcheck(dvm_diagnostic_request(NULL, 17, 0, &extra) == DVM_DIAG_INVALID && !extra);
    unitcheck(dvm_diagnostic_request(box, 0, 0, &extra) == DVM_DIAG_INVALID && !extra);
    unitcheck(dvm_diagnostic_request(box, 17, 0, NULL) == DVM_DIAG_INVALID);
    unitreply(box, 0, DVM_DIAG_INVALID, 0);
    for (index = 0; index < 8; index++)
    {
        unitcheck(dvm_diagnostic_request(box, 17, index, &tickets[index]) == DVM_DIAG_READY);
    }
    for (index = 0; index < 4; index++)
    {
        __real_dvm_diagnostic_answer(box, &sample, 2);
    }
    unitcheck(!__real_dvm_diagnostic_pending(box));
    unitcheck(dvm_diagnostic_request(box, 17, 99, &extra) == DVM_DIAG_FULL && !extra);
    unitcheck(dvm_diagnostic_reply(box, 18, tickets[0], &output) == DVM_DIAG_DENIED && !output.owner_pid &&
              !output.cookie && !output.dispatches && !output.turns && !output.waits);
    unitcheck(dvm_diagnostic_forget(box, 18, tickets[0]) == DVM_DIAG_DENIED);
    for (index = 0; index < 8; index++)
    {
        unitreply(box, tickets[index], DVM_DIAG_READY, index);
    }
    for (index = 0; index < 4; index++)
    {
        unitcheck(dvm_diagnostic_request(box, 17, 100 + index, &tickets[index]) == DVM_DIAG_READY);
    }
    __real_dvm_diagnostic_answer(box, &sample, 0);
    __real_dvm_diagnostic_answer(box, &sample, 3);
    unitreply(box, tickets[0], DVM_DIAG_PENDING, 0);
    __real_dvm_diagnostic_answer(box, &sample, 2);
    unitreply(box, tickets[0], DVM_DIAG_READY, 100);
    unitreply(box, tickets[1], DVM_DIAG_READY, 101);
    for (index = 0; index < 2; index++)
    {
        unitcheck(dvm_diagnostic_request(box, 17, 200 + index, &later[index]) == DVM_DIAG_READY);
        unitcheck(later[index] > tickets[3]);
    }
    __real_dvm_diagnostic_answer(box, &sample, 2);
    unitreply(box, later[0], DVM_DIAG_PENDING, 0);
    unitreply(box, tickets[2], DVM_DIAG_READY, 102);
    unitreply(box, tickets[3], DVM_DIAG_READY, 103);
    __real_dvm_diagnostic_answer(box, &sample, 2);
    unitreply(box, later[0], DVM_DIAG_READY, 200);
    unitreply(box, later[1], DVM_DIAG_READY, 201);
    unitcheck(dvm_diagnostic_request(box, 17, 55, &extra) == DVM_DIAG_READY);
    unitcheck(dvm_diagnostic_forget(box, 17, extra) == DVM_DIAG_READY);
    unitreply(box, extra, DVM_DIAG_MISSING, 0);
    unitcheck(!__real_dvm_diagnostic_pending(box));
    unitcheck(dvm_diagnostic_request(box, 17, 66, &tickets[0]) == DVM_DIAG_READY);
    unitcheck(dvm_diagnostic_request(box, 17, 77, &tickets[1]) == DVM_DIAG_READY);
    __real_dvm_diagnostic_answer(box, &sample, 1);
    dvm_diagnostic_close(box);
    dvm_diagnostic_close(box);
    unitreply(box, tickets[0], DVM_DIAG_READY, 66);
    unitreply(box, tickets[1], DVM_DIAG_CLOSED, 0);
    unitcheck(!__real_dvm_diagnostic_pending(box));
    unitcheck(dvm_diagnostic_request(box, 17, 88, &extra) == DVM_DIAG_CLOSED && !extra);
    dvm_diagnostic_destroy(box);
    fprintf(stderr, "NATIVEDEBUGUNIT quota=1 fifo=1 close=1\n");
}


/* EN/RU: Only omit the diagnostic wake predicate, never ordinary channel readiness.
 * Убрать только diagnostic wake predicate, не ordinary channel readiness. */
int __wrap_dvm_diagnostic_pending(DVM_DiagnosticMailbox *box)
{
    return CONTROL == 1 ? 0 : __real_dvm_diagnostic_pending(box);
}


/* EN/RU: Omit requester cleanup only; underlying native slots still destroy normally.
 * Убрать requester cleanup; native slots по-прежнему уничтожаются штатно. */
void __wrap_dvm_diagnostic_forget_requester(DVM_DiagnosticMailbox *box, int requester)
{
    if (CONTROL != 2)
    {
        __real_dvm_diagnostic_forget_requester(box, requester);
    }
}


/* EN: Borrow this owner's own native Prog identity through callback. Never read
 * foreign state; verify bound TLS and at most two answers before delegating.
 * RU: Заимствовать собственный native Prog identity owner до возврата callback.
 * Не читать foreign state; проверить TLS и максимум два answers до delegation. */
void __wrap_dvm_diagnostic_answer(DVM_DiagnosticMailbox *box, const DVM_DiagnosticSample *sample, unsigned int budget)
{
    Prog *own = dvm_managed_workers_guest();
    if (!own || !dvm_native_context_current() || sample->owner_pid != own->pid || !budget || budget > 2)
    {
        fprintf(stderr, "FAIL diagnostic serviced on foreign owner\n");
        exit(14);
    }
    if (!__atomic_exchange_n(&observed, 1, __ATOMIC_RELAXED))
    {
        native_queue_unit();
        fprintf(stderr, "NATIVEDEBUGOWNER bound=1 budget=2\n");
    }
    __real_dvm_diagnostic_answer(box, sample, budget);
}
