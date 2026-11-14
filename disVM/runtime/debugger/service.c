/* Copyright (c) 2026 bhgv
 * EN: Owner-local debug publications and native reentry gates; no managed ownership transfer.
 * RU: Debug publications своего owner и native reentry gates; без передачи managed ownership.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/random.h>
#include <errno.h>
#include <limits.h>

int dvm_debug_enabled;                /* Immutable during VM owner lifetime. */
DebugService dvm_debug_service;       /* Native registry, initialized before VM startup. */
static __thread DebugTarget *current; /* Permanent source owner only; never migrates. */


/* EN: Generate 128 random session bits before startup; zero on OS entropy failure.
 * RU: Создать 128 random session bits до startup; ноль при отказе OS entropy. */
int dvm_debug_session_init(void)
{
    unsigned char bytes[16];
    size_t used = 0;
    while (used < sizeof(bytes))
    {
        ssize_t count = getrandom(bytes + used, sizeof(bytes) - used, 0);
        if (count < 0 && errno == EINTR)
        {
            continue;
        }
        if (count <= 0)
        {
            return 0;
        }
        used += (size_t)count;
    }
    for (used = 0; used < sizeof(bytes); used++)
    {
        snprintf(dvm_debug_service.session + used * 2, 3, "%02x", bytes[used]);
    }
    return 1;
}


/* EN: Bind this OS owner once to pid. Allocate native storage; return0 on failure.
 * RU: Привязать OS owner к pid один раз. Native storage; вернуть0 при отказе. */
int dvm_debug_bind(int pid)
{
    DebugTarget *target;
    if (!dvm_debug_enabled)
    {
        return 1;
    }
    if (current)
    {
        return current->pid == pid;
    }
    target = calloc(1, sizeof(*target));
    if (!target)
    {
        return 0;
    }
    target->wake = fvm_port_sync_create();
    if (!target->wake)
    {
        free(target);
        return 0;
    }
    target->pid = pid;
    fvm_port_sync_lock(dvm_debug_service.lock);
    target->next = dvm_debug_service.targets;
    dvm_debug_service.targets = target;
    fvm_port_sync_unlock(dvm_debug_service.lock);
    current = target;
    __atomic_fetch_add(&dvm_debug_service.made, 1, __ATOMIC_RELAXED);
    return 1;
}


/* EN: Remove this source card before process retirement; registry pins all readers.
 * RU: Удалить свою card до retirement process; registry удерживает всех readers. */
void dvm_debug_retire(void)
{
    DebugTarget **link;
    DebugTarget *target = current;
    if (!target)
    {
        return;
    }
    fvm_port_sync_lock(dvm_debug_service.lock);
    for (link = &dvm_debug_service.targets; *link && *link != target; link = &(*link)->next)
    {
    }
    if (*link)
    {
        *link = target->next;
    }
    fvm_port_sync_unlock(dvm_debug_service.lock);
    current = NULL;
    fvm_port_sync_destroy(target->wake);
    free(target);
    __atomic_fetch_add(&dvm_debug_service.freed, 1, __ATOMIC_RELAXED);
}


/* EN: Copy borrowed source REG/op/location before execution; no live managed reads by peers.
 * RU: Копировать свои REG/op/location до execution; peers не читают live managed memory. */
void dvm_debug_instruction(const REG *registers, const Inst *instruction, const DVM_DiagnosticLocation *location)
{
    if (!current)
    {
        return;
    }
    fvm_port_sync_lock(current->wake);
    current->registers = *registers;
    current->instruction = *instruction;
    current->location = *location;
    current->boundary = 1;
    current->fault = 0;
    current->error[0] = 0;
    if (current->step_after)
    {
        current->step_after = 0;
        current->requested = 1;
    }
    fvm_port_sync_unlock(current->wake);
}


/* EN/RU: Source leaves its pre-instruction boundary; saturate debug-only step count.
 * Source покидает pre-instruction boundary; насыщать debug-only step count. */
void dvm_debug_instruction_enter(void)
{
    if (current)
    {
        fvm_port_sync_lock(current->wake);
        current->boundary = 0;
        if (current->steps < ULONG_MAX)
        {
            current->steps++;
        }
        fvm_port_sync_unlock(current->wake);
    }
}


/* EN: Keep the original bounded error before handler. Source alone owns message.
 * RU: Сохранить bounded original error до handler. Message принадлежит source. */
void dvm_debug_fault(const char *message)
{
    if (current)
    {
        fvm_port_sync_lock(current->wake);
        snprintf(current->error, sizeof(current->error), "%s", message);
        current->fault = 1;
        current->requested = 1;
        current->boundary = 0;
        fvm_port_sync_unlock(current->wake);
        __atomic_fetch_add(&dvm_debug_service.faults, 1, __ATOMIC_RELAXED);
    }
}


/* EN/RU: Record refusal honestly; no frozen memory claim follows this publication.
 * Отметить отказ честно; publication не обещает frozen memory. */
void dvm_debug_stop_refused(void)
{
    if (current)
    {
        fvm_port_sync_lock(current->wake);
        current->refused = 1;
        current->requested = 0;
        fvm_port_sync_unlock(current->wake);
    }
}


/* EN/RU: Read this owner's synchronized pause flag; zero outside debug mode.
 * Прочитать pause flag своего owner под lock; ноль вне debug mode. */
int dvm_debug_requested(void)
{
    int result = 0;
    if (current)
    {
        fvm_port_sync_lock(current->wake);
        result = current->requested;
        fvm_port_sync_unlock(current->wake);
    }
    return result;
}


/* EN/RU: Read this owner's synchronized termination flag; no foreign Prog access.
 * Прочитать termination flag своего owner под lock; без foreign Prog access. */
int dvm_debug_terminated(void)
{
    int result = 0;
    if (current)
    {
        fvm_port_sync_lock(current->wake);
        result = current->terminate;
        fvm_port_sync_unlock(current->wake);
    }
    return result;
}


/* [GC:] EN: Publish borrowed process/REG only after source NATIVE release. Its
 * original continuation/roots remain alive; inspector access holds wake.
 * RU: Публиковать process/REG только после source NATIVE release. Original
 * continuation/roots живы; inspector access удерживает wake. */
void dvm_debug_native_publish(Prog *process, const REG *registers)
{
    if (current)
    {
        fvm_port_sync_lock(current->wake);
        current->process = process;
        current->registers = *registers;
        current->native_ready = 1;
        if (current->requested)
        {
            current->state = current->fault ? DebugFaulted : DebugPaused;
        }
        fvm_port_sync_unlock(current->wake);
    }
}


/* [GC:] EN: Wait before NATIVE reentry, preserving the source C/TLS continuation.
 * Clear borrowed publication before ACTIVE; never reload REG from a snapshot.
 * RU: Ждать до NATIVE reentry, сохраняя C/TLS continuation source. Снять borrowed
 * publication до ACTIVE; не загружать REG из snapshot. */
void dvm_debug_native_gate(Prog *process)
{
    if (current)
    {
        fvm_port_sync_lock(current->wake);
        while (current->process == process && current->requested && !current->closing && !current->terminate)
        {
            fvm_port_sync_wait(current->wake);
        }
        current->native_ready = 0;
        current->process = NULL;
        current->state = DebugRunning;
        fvm_port_sync_unlock(current->wake);
    }
}


/* EN/RU: Count a continued unhandled error without failing unrelated guests.
 * Учесть continued unhandled error без остановки остальных guests. */
void dvm_debug_unhandled(void)
{
    __atomic_fetch_add(&dvm_debug_service.unhandled, 1, __ATOMIC_RELAXED);
}


/* EN/RU: Return nonzero when continued uncaught errors must affect final app status.
 * Вернуть ненулевой status при continued uncaught errors приложения. */
int dvm_debug_exit_status(void)
{
    return dvm_debug_enabled && __atomic_load_n(&dvm_debug_service.unhandled, __ATOMIC_RELAXED) != 0;
}
