/* Copyright (c) 2026 bhgv
 * [GC:] EN: Private omissions of remote Retain and finalizer Release obligations.
 * RU: Приватные пропуски remote Retain и finalizer Release obligations.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "isa.h"
#include "sys_capture.h"
#include "remote_managed.h"
#include "proxy_gc_queue.h"
#include <string.h>

#if CONTROL == 1


/* EN/RU: Skip counted Retain across the native adapter boundary; keep successful status.
 * Пропустить counted Retain на native adapter boundary; сохранить successful status. */
DvmRemoteStatus __wrap_dvm_remote_managed_retain(DvmRemoteManaged *adapter, DvmRemoteHandle handle)
{
    USED(adapter);
    (void)handle;
    return DVM_REMOTE_OK;
}
#endif

#if CONTROL == 2
void __real_dvm_proxy_lease_retire(DvmProxyLeaseQueue *, size_t, uint64_t);


/* [GC:] EN: Restore pre-Retain state just before actual token finalization, losing
 * only its remote Release obligation. All native token/queue cleanup still runs.
 * RU: Перед token finalization вернуть pre-Retain state, потеряв лишь remote
 * Release obligation. Native token/queue cleanup по-прежнему выполняется. */
void __wrap_dvm_proxy_lease_retire(DvmProxyLeaseQueue *queue, size_t slot, uint64_t serial)
{
    pthread_mutex_lock(&queue->mutex);
    if (slot < queue->limit && queue->entries[slot].serial == serial &&
        queue->entries[slot].state == DVM_PROXY_LEASE_ACTIVE)
    {
        queue->entries[slot].state = DVM_PROXY_LEASE_STAGED;
    }
    pthread_mutex_unlock(&queue->mutex);
    __real_dvm_proxy_lease_retire(queue, slot, serial);
}
#endif

#if CONTROL == 3
#include "proxy_gc.h"


const char *__real_dvm_runtime_movp_copy(const void *, void *, void **);
static int refused; /* Atomic one-shot publication failure. */


/* [GC:] EN: Refuse the first native lease/connection publication before effects.
 * Source is the constructor's private rooted cell; other publications are real.
 * RU: Отказать первой native lease/connection publication до effects. Source —
 * private rooted cell конструктора; остальные publications настоящие. */
const char *__wrap_dvm_runtime_movp_copy(const void *source, void *destination, void **previous)
{
    void *value;
    memmove(&value, source, sizeof(value));
    if (value != H && value && dvm_proxy_gc_eligible(D2H(value)) && !__atomic_exchange_n(&refused, 1, __ATOMIC_RELAXED))
    {
        return "proxy image publication refusal";
    }
    return __real_dvm_runtime_movp_copy(source, destination, previous);
}
#endif

#if CONTROL == 4
#include "proxy_gc.h"


void __real_ptradd(Heap *);
static int refused; /* Atomic one-shot external root registration failure. */


/* [GC:] EN: Refuse a constructor root before registration; cleanup may root RC later.
 * RU: Отказать root конструктора до registration; cleanup позже укореняет RC. */
void __wrap_ptradd(Heap *heap)
{
    if (dvm_proxy_gc_eligible(heap) && !__atomic_exchange_n(&refused, 1, __ATOMIC_RELAXED))
    {
        error("proxy image publication refusal");
    }
    __real_ptradd(heap);
}
#endif

#if CONTROL == 5
uint64_t __real_dvm_proxy_lease_new(DvmProxyLeaseQueue *, DvmRemoteHandle, int, size_t *);


/* [GC:] EN/RU: Refuse only unknown-result reservations before remote callback effects.
 * Отказать лишь unknown-result reservations до remote callback effects. */
uint64_t __wrap_dvm_proxy_lease_new(DvmProxyLeaseQueue *queue, DvmRemoteHandle owner, int active, size_t *slot)
{
    return active < 0 ? 0 : __real_dvm_proxy_lease_new(queue, owner, active, slot);
}
#endif

#if CONTROL == 6


/* EN/RU: Report false binding success without committing exact returned owner metadata.
 * Ложно сообщить binding success без фиксации exact returned owner metadata. */
int __wrap_dvm_proxy_lease_bind(DvmProxyLeaseQueue *queue, size_t slot, uint64_t serial, DvmRemoteHandle owner)
{
    USED(queue);
    USED(slot);
    USED(serial);
    (void)owner;
    return 1;
}
#endif
