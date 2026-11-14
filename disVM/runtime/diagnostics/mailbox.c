/* Copyright (c) 2026 bhgv
 * EN: Fixed diagnostic credits and requester ownership, without managed heap access.
 * RU: Фиксированные diagnostic credits и requester ownership без managed heap access.
 */
#include "mailbox.h"
#include "channel_sync.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define Credits 8 /* Fixed storage and total pending/ready capacity per resident. */

enum
{
    SlotFree,    /* No request owns this native credit. */
    SlotPending, /* Original owner has not supplied its snapshot. */
    SlotReady,   /* Copied reply awaits consumption. */
    SlotClosed   /* Owner retired before snapshot. */
};


/* EN/RU: Native slot state is protected by its mailbox mutex.
 * Native slot state защищён mailbox mutex. */
typedef struct DiagnosticSlot
{
    int state;                   /* Zero free, one pending, two ready, three closed. */
    int requester;               /* Immutable guest PID while this ticket is occupied. */
    unsigned long ticket;        /* Monotonic identity, never reused in a mailbox. */
    DVM_DiagnosticSample sample; /* Copied cookie and owner result. */
} DiagnosticSlot;

struct DVM_DiagnosticMailbox
{
    FVM_PortSync *sync;            /* Own native metadata mutex; never ordinary message queue. */
    unsigned long issued;          /* Last ticket, saturating at signed-big max. */
    int closed;                    /* Owner retirement latch. */
    unsigned int waiting;          /* Atomic pending hint; updates serialized by box mutex. */
    DiagnosticSlot slots[Credits]; /* Bounded native credits. */
};

static unsigned long created;   /* Atomic successful mailbox allocations. */
static unsigned long destroyed; /* Atomic mailbox destructions after owner join. */
static unsigned long requests;  /* Atomic occupied pending/ready/closed credits. */
static unsigned long pending;   /* Atomic unserviced pending requests. */


/* EN/RU: Caller owns box lock; clear one occupied slot and adjust native counters.
 * Caller удерживает box lock; очистить occupied slot и обновить native counters. */
static void release_slot(DVM_DiagnosticMailbox *box, DiagnosticSlot *slot)
{
    if (slot->state == SlotPending)
    {
        __atomic_fetch_sub(&pending, 1, __ATOMIC_RELAXED);
        __atomic_fetch_sub(&box->waiting, 1, __ATOMIC_RELEASE);
    }
    if (slot->state)
    {
        __atomic_fetch_sub(&requests, 1, __ATOMIC_RELAXED);
    }
    memset(slot, 0, sizeof(*slot));
}


/* EN/RU: Allocate fixed native storage and its mutex; no managed allocation.
 * Выделить fixed native storage и mutex; без managed allocation. */
DVM_DiagnosticMailbox *dvm_diagnostic_create(void)
{
    DVM_DiagnosticMailbox *box = calloc(1, sizeof(*box));
    if (!box)
    {
        return NULL;
    }
    box->sync = fvm_port_sync_create();
    if (!box->sync)
    {
        free(box);
        return NULL;
    }
    __atomic_fetch_add(&created, 1, __ATOMIC_RELAXED);
    return box;
}


/* EN/RU: Caller has joined owner and excluded other users; NULL is accepted.
 * Caller присоединил owner и исключил других users; NULL допустим. */
void dvm_diagnostic_destroy(DVM_DiagnosticMailbox *box)
{
    unsigned int index;
    if (!box)
    {
        return;
    }
    for (index = 0; index < Credits; index++)
    {
        release_slot(box, &box->slots[index]);
    }
    fvm_port_sync_destroy(box->sync);
    free(box);
    __atomic_fetch_add(&destroyed, 1, __ATOMIC_RELAXED);
}


/* EN/RU: Accept one private scalar and nonzero requester; refusal zeros ticket.
 * Принять private scalar и nonzero requester; отказ обнуляет ticket. */
int dvm_diagnostic_request(DVM_DiagnosticMailbox *box, int requester, int64_t cookie, unsigned long *ticket)
{
    unsigned int index;
    int result = DVM_DIAG_FULL;
    if (!ticket)
    {
        return DVM_DIAG_INVALID;
    }
    *ticket = 0;
    if (!box || requester <= 0)
    {
        return DVM_DIAG_INVALID;
    }
    fvm_port_sync_lock(box->sync);
    if (box->closed)
    {
        result = DVM_DIAG_CLOSED;
    }
    else if (box->issued == (unsigned long)LLONG_MAX)
    {
        result = DVM_DIAG_EXHAUSTED;
    }
    else
    {
        for (index = 0; index < Credits; index++)
        {
            DiagnosticSlot *slot = &box->slots[index];
            if (!slot->state)
            {
                slot->ticket = ++box->issued;
                slot->requester = requester;
                slot->sample.cookie = cookie;
                slot->state = SlotPending;
                *ticket = slot->ticket;
                __atomic_fetch_add(&requests, 1, __ATOMIC_RELAXED);
                __atomic_fetch_add(&pending, 1, __ATOMIC_RELAXED);
                __atomic_fetch_add(&box->waiting, 1, __ATOMIC_RELEASE);
                result = DVM_DIAG_READY;
                break;
            }
        }
    }
    fvm_port_sync_unlock(box->sync);
    return result;
}


/* EN/RU: With live box pinned, read or forget own ticket under its mutex.
 * При pinned live box прочитать либо забыть свой ticket под mutex. */
static int
take_ticket(DVM_DiagnosticMailbox *box, int requester, unsigned long ticket, DVM_DiagnosticSample *sample, int forget)
{
    unsigned int index;
    int result = DVM_DIAG_MISSING;
    memset(sample, 0, sizeof(*sample));
    if (!box || requester <= 0 || !ticket || ticket > (unsigned long)LLONG_MAX)
    {
        return DVM_DIAG_INVALID;
    }
    fvm_port_sync_lock(box->sync);
    for (index = 0; index < Credits; index++)
    {
        DiagnosticSlot *slot = &box->slots[index];
        if (slot->state && slot->ticket == ticket)
        {
            result = DVM_DIAG_DENIED;
            if (slot->requester == requester)
            {
                result = DVM_DIAG_PENDING;
                if (forget || slot->state != SlotPending)
                {
                    result = slot->state == SlotClosed && !forget ? DVM_DIAG_CLOSED : DVM_DIAG_READY;
                    if (!forget && result == DVM_DIAG_READY)
                    {
                        *sample = slot->sample;
                    }
                    release_slot(box, slot);
                }
            }
            break;
        }
    }
    fvm_port_sync_unlock(box->sync);
    return result;
}


/* EN/RU: Consume only complete replies; validate caller-owned native output.
 * Поглотить только complete replies; проверить caller-owned native output. */
int dvm_diagnostic_reply(DVM_DiagnosticMailbox *box, int requester, unsigned long ticket, DVM_DiagnosticSample *sample)
{
    return sample ? take_ticket(box, requester, ticket, sample, 0) : DVM_DIAG_INVALID;
}


/* EN/RU: Forget pending/ready/closed own ticket; no ordinary-operation mutation.
 * Забыть свой pending/ready/closed ticket; обычная операция не меняется. */
int dvm_diagnostic_forget(DVM_DiagnosticMailbox *box, int requester, unsigned long ticket)
{
    DVM_DiagnosticSample ignored;
    return take_ticket(box, requester, ticket, &ignored, 1);
}


/* EN/RU: Release all credits of requester during its retirement; box is pinned.
 * Освободить credits requester при retirement; box удерживается caller. */
void dvm_diagnostic_forget_requester(DVM_DiagnosticMailbox *box, int requester)
{
    unsigned int index;
    fvm_port_sync_lock(box->sync);
    for (index = 0; index < Credits; index++)
    {
        if (box->slots[index].state && box->slots[index].requester == requester)
        {
            release_slot(box, &box->slots[index]);
        }
    }
    fvm_port_sync_unlock(box->sync);
}


/* EN/RU: Retire pending requests without discarding already completed snapshots.
 * Завершить pending requests без удаления готовых snapshots. */
void dvm_diagnostic_close(DVM_DiagnosticMailbox *box)
{
    unsigned int index;
    fvm_port_sync_lock(box->sync);
    box->closed = 1;
    for (index = 0; index < Credits; index++)
    {
        if (box->slots[index].state == SlotPending)
        {
            box->slots[index].state = SlotClosed;
            __atomic_fetch_sub(&pending, 1, __ATOMIC_RELAXED);
            __atomic_fetch_sub(&box->waiting, 1, __ATOMIC_RELEASE);
        }
    }
    fvm_port_sync_unlock(box->sync);
}


/* EN/RU: Copy bounded pending predicate; no message is removed or serviced.
 * Прочитать bounded pending predicate; сообщения не извлекаются/обрабатываются. */
int dvm_diagnostic_pending(DVM_DiagnosticMailbox *box)
{
    return __atomic_load_n(&box->waiting, __ATOMIC_ACQUIRE) != 0;
}


/* EN/RU: Owner supplies native sample; oldest pending ticket wins each bounded step.
 * Owner передаёт native sample; на каждом bounded шаге выбрать старейший ticket. */
void dvm_diagnostic_answer(DVM_DiagnosticMailbox *box, const DVM_DiagnosticSample *sample, unsigned int budget)
{
    unsigned int step;
    if (!sample || budget > 2)
    {
        return;
    }
    fvm_port_sync_lock(box->sync);
    for (step = 0; step < budget; step++)
    {
        unsigned int index;
        DiagnosticSlot *selected = NULL;
        for (index = 0; index < Credits; index++)
        {
            DiagnosticSlot *slot = &box->slots[index];
            if (slot->state == SlotPending && (!selected || slot->ticket < selected->ticket))
            {
                selected = slot;
            }
        }
        if (!selected)
        {
            break;
        }
        int64_t cookie = selected->sample.cookie;
        selected->sample = *sample;
        selected->sample.cookie = cookie;
        selected->state = SlotReady;
        __atomic_fetch_sub(&pending, 1, __ATOMIC_RELAXED);
        __atomic_fetch_sub(&box->waiting, 1, __ATOMIC_RELEASE);
    }
    fvm_port_sync_unlock(box->sync);
}


/* EN/RU: Read the shared credit count; caller obeys single-owner retirement ordering.
 * Прочитать credit count; caller соблюдает single-owner retirement ordering. */
int dvm_diagnostic_has_requests(void)
{
    return __atomic_load_n(&requests, __ATOMIC_ACQUIRE) != 0;
}


/* EN/RU: Read atomic counts; runtime invokes after all mailbox owners join.
 * Прочитать atomic counts; runtime вызывает после join всех mailbox owners. */
void dvm_diagnostic_report(void)
{
    unsigned long made = __atomic_load_n(&created, __ATOMIC_RELAXED);
    unsigned long freed = __atomic_load_n(&destroyed, __ATOMIC_RELAXED);
    fprintf(stderr,
            "NATIVEDEBUGMAILBOX created=%lu destroyed=%lu live=%lu requests=%lu pending=%lu\n",
            made,
            freed,
            made - freed,
            __atomic_load_n(&requests, __ATOMIC_RELAXED),
            __atomic_load_n(&pending, __ATOMIC_RELAXED));
}
