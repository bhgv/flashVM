/* Copyright (c) 2026 bhgv
 * [GC:] EN: Guest owner session lifecycle, independent roots and retirement cleanup.
 * RU: Жизнь guest owner session, отдельные roots и cleanup при retirement.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "raise.h"
#include "proxyhost.h"
#include "proxyhost_decl.h"
#include "proxyhost_table.h"
#include "sys_capture.h"
#include "disvm/native_owner.h"
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

__thread DvmProxyHostState dvm_proxyhost_state; /* One session per permanent guest owner. */
static unsigned long opened;                    /* Atomic cumulative successful opens. */
static unsigned long closed;                    /* Atomic cumulative completed cleanup. */
static unsigned long exports;                   /* Atomic outstanding rooted exports, including pending transfer. */
static unsigned long active;                    /* Atomic admitted owner callbacks, not channel deliveries. */


/* EN/RU: Exact scalar-only generated publication result, with session fencing.
 * Точный scalar-only generated publication result с session fencing. */
typedef struct
{
    WORD status;     /* Publication outcome. */
    LONG session;    /* Process-local session bit identity, not authentication. */
    WORD slot;       /* Bounded object index. */
    LONG generation; /* Nonzero object incarnation. */
} PublishResult;


/* EN/RU: Record exact admitted/completed request counts; delta is +1 or -1.
 * Учесть admitted/completed requests; delta равен +1 либо -1. */
void dvm_proxyhost_active(int delta)
{
    if (delta > 0)
    {
        __atomic_fetch_add(&active, 1, __ATOMIC_RELAXED);
    }
    else if (__atomic_fetch_sub(&active, 1, __ATOMIC_RELAXED) == 0)
    {
        abort();
    }
}


/* [GC:] EN: Collect an owner-local bounded slice; capacity preserves pending roots.
 * RU: Собрать owner-local bounded slice; capacity сохраняет pending roots. */
static DvmRemoteStatus collect_roots(void)
{
    size_t released = 0;
    DvmRemoteStatus status =
        dvm_remote_managed_collect(dvm_proxyhost_state.adapter, dvm_proxyhost_state.objects, &released);
    if (__atomic_fetch_sub(&exports, released, __ATOMIC_RELAXED) < released)
    {
        abort();
    }
    return status;
}


/* EN: Open bounded native state on this ACTIVE owner; limits prevent wire-sized
 * allocations. Captured scalar return remains caller-owned through completion.
 * RU: Открыть bounded state на ACTIVE owner; limits не дают огромных allocations.
 * Captured scalar result принадлежит caller до завершения. */
void Proxyhost_open(void *frame)
{
    F_Proxyhost_open call;
    WORD status = DVM_REMOTE_INVALID;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Proxyhost_open, ret), sizeof(*call.ret));
    if (!dvm_proxyhost_state.adapter && call.objects > 0 && call.objects <= 256 && call.requests > 0 &&
        call.requests <= 4096)
    {
        dvm_proxyhost_state.adapter = dvm_remote_managed_new(call.objects, call.requests, UINT64_MAX);
        if (dvm_proxyhost_state.adapter)
        {
            dvm_proxyhost_state.objects = call.objects;
            status = DVM_REMOTE_OK;
            __atomic_fetch_add(&opened, 1, __ATOMIC_RELAXED);
        }
        else
        {
            status = DVM_REMOTE_CAPACITY;
        }
    }
    memmove(call.ret, &status, sizeof(status));
}


/* EN/RU: Permit waiting only outside active owner execution; scalar TLS read.
 * Разрешить ожидание только вне active owner execution; scalar TLS read. */
void Proxyhost_canwait(void *frame)
{
    F_Proxyhost_canwait call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Proxyhost_canwait, ret), sizeof(*call.ret));
    WORD permitted = dvm_proxyhost_state.active == 0;
    memmove(call.ret, &permitted, sizeof(permitted));
}


/* [GC:] EN: Publish typed Cell input with separate root/ref ownership. Return
 * status/session/slot/generation only; no managed pointer leaves the owner channel API.
 * RU: Опубликовать typed Cell с root/ref. Вернуть status/session/slot/generation;
 * managed pointer не передаётся через owner channel API. */
void Proxyhost_publish(void *frame)
{
    F_Proxyhost_publish call;
    PublishResult result = {DVM_REMOTE_INVALID, 0, 0, 0};
    DvmRemoteHandle handle;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Proxyhost_publish, ret), sizeof(*call.ret));
    if (sizeof(*call.ret) != sizeof(result))
    {
        error(exBounds);
    }
    void *cell = dvm_sys_owner(&call.cell);
    if (dvm_proxyhost_state.adapter && cell != H && D2H(cell)->t->size == sizeof(Proxyhost_Cell) &&
        D2H(cell)->t->np == 0 && (call.kind == DVM_REMOTE_VARIABLE || call.kind == DVM_REMOTE_FUNCTION))
    {
        result.status = dvm_remote_managed_publish(dvm_proxyhost_state.adapter, call.kind, cell, &handle);
        if (result.status == DVM_REMOTE_OK)
        {
            dvm_proxyhost_state.identity = handle.session;
            memmove(&result.session, &handle.session, sizeof(result.session));
            result.slot = handle.slot;
            result.generation = handle.generation;
            __atomic_fetch_add(&exports, 1, __ATOMIC_RELAXED);
        }
    }
    memmove(call.ret, &result, sizeof(result));
}


/* EN/RU: Finish exact active call after callback or handled error, then collect.
 * Завершить точный active call после callback/handled error, затем collect. */
void Proxyhost_finish(void *frame)
{
    F_Proxyhost_finish call;
    WORD status = DVM_REMOTE_STALE;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Proxyhost_finish, ret), sizeof(*call.ret));
    if (call.id > 0 && (uint64_t)call.id == dvm_proxyhost_state.active)
    {
        status = dvm_remote_managed_finish(dvm_proxyhost_state.adapter, call.id);
        if (status == DVM_REMOTE_OK)
        {
            dvm_proxyhost_state.active = 0;
            dvm_proxyhost_active(-1);
            (void)collect_roots();
        }
    }
    memmove(call.ret, &status, sizeof(status));
}


/* EN/RU: Execute trusted owner-local Release; collection pressure is deferred,
 * not a reason to replay the completed Release. Retain/replay belongs above API.
 * Выполнить owner-local Release; pressure не разрешает повторить готовый effect. */
void Proxyhost_drop(void *frame)
{
    F_Proxyhost_drop call;
    WORD status = DVM_REMOTE_INVALID;
    DvmRemoteHandle handle = {dvm_proxyhost_state.identity, 0, 0};
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Proxyhost_drop, ret), sizeof(*call.ret));
    if (dvm_proxyhost_state.adapter && (uint64_t)call.session == dvm_proxyhost_state.identity && call.slot >= 0 &&
        call.generation > 0)
    {
        handle.slot = call.slot;
        handle.generation = call.generation;
        status = dvm_remote_managed_release(dvm_proxyhost_state.adapter, handle);
        (void)collect_roots();
    }
    memmove(call.ret, &status, sizeof(status));
}


/* [GC:] EN: Close only after the active callback ends; capacity keeps TLS state
 * and roots intact for retry. Successful destroy clears every native identity.
 * RU: Закрыть после callback; capacity сохраняет TLS/roots для retry. Успех
 * destroy очищает все native identities. No waits or destructors under locks. */
static DvmRemoteStatus close_session(void)
{
    if (!dvm_proxyhost_state.adapter)
    {
        return DVM_REMOTE_OK;
    }
    if (dvm_proxyhost_state.active)
    {
        return DVM_REMOTE_BUSY;
    }
    dvm_remote_managed_close(dvm_proxyhost_state.adapter);
    DvmRemoteStatus status = collect_roots();
    if (status == DVM_REMOTE_OK)
    {
        status = dvm_remote_managed_destroy(dvm_proxyhost_state.adapter);
    }
    if (status == DVM_REMOTE_OK)
    {
        memset(&dvm_proxyhost_state, 0, sizeof(dvm_proxyhost_state));
        __atomic_fetch_add(&closed, 1, __ATOMIC_RELAXED);
    }
    return status;
}


/* EN/RU: Synchronous close result; guest may sleep/retry Capacity on this owner.
 * Синхронный close status; guest может sleep/retry Capacity на этом owner. */
void Proxyhost_close(void *frame)
{
    F_Proxyhost_close call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Proxyhost_close, ret), sizeof(*call.ret));
    WORD status = close_session();
    memmove(call.ret, &status, sizeof(status));
}


/* [GC:] EN: Terminal owner cleanup after its stack/REG snapshot is published.
 * Finish abandoned admitted work exactly once; only this owner leaves ACTIVE
 * during queue pressure. Other guests/collectors remain runnable throughout.
 * RU: Terminal cleanup после публикации stack/REG. Завершить abandoned call один
 * раз; при pressure только этот owner выходит из ACTIVE, остальные работают. */
void dvm_proxyhost_retire(void)
{
    if (dvm_proxyhost_state.active)
    {
        if (dvm_remote_managed_finish(dvm_proxyhost_state.adapter, dvm_proxyhost_state.active) != DVM_REMOTE_OK)
        {
            abort();
        }
        dvm_proxyhost_state.active = 0;
        dvm_proxyhost_active(-1);
    }
    DvmRemoteStatus status;
    while ((status = close_session()) == DVM_REMOTE_CAPACITY)
    {
        DVM_HeapOwner *owner = dvm_native_owner_current();
        if (!dvm_native_owner_leave(owner))
        {
            abort();
        }
        (void)poll(NULL, 0, 1);
        if (!dvm_native_owner_enter(owner))
        {
            abort();
        }
    }
    if (status != DVM_REMOTE_OK)
    {
        abort();
    }
}


/* EN/RU: Register the matched generated builtin table before guest startup.
 * Зарегистрировать matching generated builtin table до старта гостей. */
void dvm_proxyhost_initialize(void)
{
    builtinmod("$Proxyhost", Proxyhostmodtab, Proxyhostmodlen);
}


/* EN/RU: Print immutable joined lifecycle evidence; no managed access occurs.
 * Напечатать joined lifecycle evidence без managed access. */
void dvm_proxyhost_statistics(void)
{
    unsigned long total = __atomic_load_n(&opened, __ATOMIC_ACQUIRE);
    if (total)
    {
        printf("PROXYHOST opened=%lu closed=%lu exports=%lu active=%lu\n",
               total,
               __atomic_load_n(&closed, __ATOMIC_ACQUIRE),
               __atomic_load_n(&exports, __ATOMIC_ACQUIRE),
               __atomic_load_n(&active, __ATOMIC_ACQUIRE));
    }
}
