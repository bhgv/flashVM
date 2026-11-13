/* Copyright (c) 2026 bhgv
 * [GC:] EN: Managed native lease payloads and bounded finalizer-to-broker release queue.
 * RU: Managed native lease payloads и bounded finalizer-to-broker release queue.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "raise.h"
#include "sys_capture.h"
#include "shared_pointer.h"
#include "disvm/refcount.h"
#include "proxy_gc.h"
#include "proxy_gc_queue.h"
#include "proxygc_decl.h"
#include "proxygc_table.h"
#include <stdlib.h>
#include <string.h>


/* [GC:] EN/RU: Native connection owner; no managed children are stored in payload.
 * Native connection owner; payload не хранит managed children. */
typedef struct
{
    WORD reserved;             /* Non-authoritative public prefix. */
    DvmProxyLeaseQueue *queue; /* One independent native metadata owner. */
} Connection;


/* [GC:] EN/RU: A managed token owns one exact native slot and native queue reference.
 * Managed token владеет точным native slot и native queue reference. */
typedef struct
{
    WORD reserved;             /* Non-authoritative public prefix. */
    DvmProxyLeaseQueue *queue; /* Native metadata outlives token's finalizer. */
    size_t slot;               /* Fixed bounded slot, never a managed pointer. */
    uint64_t serial;           /* Prevents accidental slot reuse. */
} Lease;


/* EN/RU: These three generated scalar-result frames must have identical geometry.
 * Три generated scalar-result frames обязаны иметь одинаковую геометрию. */
typedef char MatchingActionFrames[(sizeof(F_Proxygc_active) == sizeof(F_Proxygc_activate) &&
                                   sizeof(F_Proxygc_active) == sizeof(F_Proxygc_consume) &&
                                   offsetof(F_Proxygc_active, lease) == offsetof(F_Proxygc_activate, lease) &&
                                   offsetof(F_Proxygc_active, lease) == offsetof(F_Proxygc_consume, lease))
                                      ? 1
                                      : -1];


static Type *connection_type; /* Immutable startup identity, held for runtime lifetime. */
static Type *lease_type;      /* Immutable pointer-free native token identity. */


/* [GC:] EN/RU: Last managed connection owner closes metadata, then drops native ownership.
 * Последний managed connection owner закрывает metadata и снимает native ownership. */
static void free_connection(Heap *heap, int swept)
{
    Connection *value = H2D(Connection *, heap);
    USED(swept);
    if (value->queue)
    {
        dvm_proxy_lease_queue_close(value->queue);
        dvm_proxy_lease_queue_drop(value->queue);
        value->queue = NULL;
    }
}


/* [GC:] EN/RU: Finalizer only mutates native metadata; no mailbox/heap/network callback.
 * Finalizer меняет лишь native metadata; без mailbox/heap/network callback. */
static void free_lease(Heap *heap, int swept)
{
    Lease *value = H2D(Lease *, heap);
    USED(swept);
    if (value->queue)
    {
        dvm_proxy_lease_retire(value->queue, value->slot, value->serial);
        value->queue = NULL;
    }
}


/* [GC:] EN/RU: Admit only exact runtime-private descriptors, never arbitrary finalizers.
 * Допустить лишь точные runtime-private descriptors, не произвольные finalizers. */
int dvm_proxy_gc_eligible(const Heap *heap)
{
    return (connection_type && heap->t == connection_type) || (lease_type && heap->t == lease_type);
}


/* [GC:] EN/RU: Claimed last-reference callback, after native reader grace.
 * Callback захваченной последней ссылки после native reader grace. */
void dvm_proxy_gc_finalize(Heap *heap)
{
    if (!dvm_proxy_gc_eligible(heap) || heap->t->np || dvm_refcount_load_u(&heap->ref) != 0)
    {
        abort();
    }
    heap->t->free(heap, 0);
}


/* [GC:] EN: Transfer a newly allocated strong owner to a guarded mapped result.
 * Root before any publication/poll, keep old owner until guarded replacement,
 * then retire displaced/private owners outside guards. Refusal preserves result.
 * RU: Передать новый strong owner в guarded mapped result. Root предшествует
 * publication/poll; old owner жив до замены. Снять displaced/private owners вне
 * guards. Отказ сохраняет result. value may be H for ordinary capacity refusal. */
static void publish(void *value, void *destination)
{
    void *previous = H;
    volatile int rooted = 0;
    if (waserror())
    {
        if (rooted)
        {
            ptrdel(D2H(value));
        }
        destroy(value);
        nexterror();
    }
    if (value != H)
    {
        ptradd(D2H(value));
        rooted = 1;
    }
    const char *problem = dvm_runtime_movp_copy(&value, destination, &previous);
    if (problem)
    {
        error((char *)problem);
    }
    destroy(previous);
    if (rooted)
    {
        ptrdel(D2H(value));
    }
    poperror();
    destroy(value);
}


/* [GC:] EN/RU: Validate exact borrowed connection argument before native metadata access.
 * Проверить exact borrowed connection до native metadata access. */
static DvmProxyLeaseQueue *connection(Proxygc_Connection *value)
{
    if (!value || value == H || D2H(value)->t != connection_type || !((Connection *)value)->queue)
    {
        error(exHeap);
    }
    return ((Connection *)value)->queue;
}


/* [GC:] EN/RU: Validate borrowed token type; nil is an inactive ordinary result.
 * Проверить borrowed token type; nil — обычный inactive результат. */
static Lease *token(Proxygc_Lease *value)
{
    if (!value || value == H)
    {
        return NULL;
    }
    if (D2H(value)->t != lease_type || !((Lease *)value)->queue)
    {
        error(exHeap);
    }
    return (Lease *)value;
}


/* EN/RU: Create fixed native queue and transfer it into one managed connection payload.
 * Создать fixed native queue и передать её managed connection payload. */
void Proxygc_open(void *frame)
{
    F_Proxygc_open call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Proxygc_open, ret), sizeof(*call.ret));
    DvmProxyLeaseQueue *queue = call.limit > 0 ? dvm_proxy_lease_queue_new((size_t)call.limit) : NULL;
    if (!queue)
    {
        publish(H, call.ret);
        return;
    }
    if (waserror())
    {
        dvm_proxy_lease_queue_close(queue);
        dvm_proxy_lease_queue_drop(queue);
        nexterror();
    }
    Connection *value = H2D(Connection *, heap(connection_type));
    value->reserved = 0;
    value->queue = queue;
    poperror();
    publish(value, call.ret);
}


/* EN/RU: Stage one exact token before any remote Retain effect; nil on bounded quota.
 * Создать exact token до remote Retain; nil при bounded quota. */
void Proxygc_make(void *frame)
{
    F_Proxygc_make call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Proxygc_make, ret), sizeof(*call.ret));
    DvmProxyLeaseQueue *queue = connection(call.connection);
    Lease *value = H2D(Lease *, heap(lease_type));
    memset(value, 0, sizeof(*value));
    if (call.session > 0 && call.slot >= 0 && call.generation > 0 && (call.active == 0 || call.active == 1))
    {
        DvmRemoteHandle owner = {(uint64_t)call.session, (uint64_t)call.generation, (size_t)call.slot};
        value->serial = dvm_proxy_lease_new(queue, owner, call.active, &value->slot);
        if (value->serial)
        {
            value->queue = queue;
        }
    }
    if (!value->queue)
    {
        destroy(value);
        publish(H, call.ret);
        return;
    }
    publish(value, call.ret);
}


/* [GC:] EN/RU: Reserve native quota and a managed result owner before wire effects.
 * Резервировать native quota и managed result owner до wire effects. */
void Proxygc_reserve(void *frame)
{
    F_Proxygc_reserve call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Proxygc_reserve, ret), sizeof(*call.ret));
    DvmProxyLeaseQueue *queue = connection(call.connection);
    Lease *value = H2D(Lease *, heap(lease_type));
    memset(value, 0, sizeof(*value));
    DvmRemoteHandle absent = {0};
    value->serial = dvm_proxy_lease_new(queue, absent, -1, &value->slot);
    if (!value->serial)
    {
        destroy(value);
        publish(H, call.ret);
        return;
    }
    value->queue = queue;
    publish(value, call.ret);
}


/* EN/RU: Bind a reserved token after checked reply; refusal cannot change metadata.
 * Связать reserved token после checked reply; отказ не меняет metadata. */
void Proxygc_bind(void *frame)
{
    F_Proxygc_bind call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Proxygc_bind, ret), sizeof(*call.ret));
    Lease *value = token(call.lease);
    WORD result = 0;
    if (value && call.session > 0 && call.slot >= 0 && call.generation > 0)
    {
        DvmRemoteHandle owner = {(uint64_t)call.session, (uint64_t)call.generation, (size_t)call.slot};
        result = dvm_proxy_lease_bind(value->queue, value->slot, value->serial, owner);
    }
    dvm_sys_scalar_result(&result, call.ret, sizeof(result));
}


/* EN/RU: Shared implementation of active/activate/consume with captured result guards.
 * Общая реализация active/activate/consume с captured result guards. */
static void action(void *frame, int operation)
{
    F_Proxygc_active call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Proxygc_active, ret), sizeof(*call.ret));
    Lease *value = token(call.lease);
    WORD result = value && dvm_proxy_lease_action(value->queue, value->slot, value->serial, operation);
    dvm_sys_scalar_result(&result, call.ret, sizeof(result));
}


/* EN/RU: Read token activation from any caller while its mapped owner stays live.
 * Читать activation с любого caller при живом mapped owner. */
void Proxygc_active(void *frame)
{
    action(frame, 0);
}


/* EN/RU: Check exact owner scalars against live native token before offer/admission.
 * Проверить owner scalars по live native token до offer/admission. */
void Proxygc_matches(void *frame)
{
    F_Proxygc_matches call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Proxygc_matches, ret), sizeof(*call.ret));
    Lease *value = token(call.lease);
    WORD result = 0;
    if (value && call.session > 0 && call.slot >= 0 && call.generation > 0)
    {
        DvmRemoteHandle owner = {(uint64_t)call.session, (uint64_t)call.generation, (size_t)call.slot};
        result = dvm_proxy_lease_matches(value->queue, value->slot, value->serial, owner);
    }
    dvm_sys_scalar_result(&result, call.ret, sizeof(result));
}


/* EN/RU: Broker commits staged token after successful remote Retain.
 * Broker фиксирует staged token после успешного remote Retain. */
void Proxygc_activate(void *frame)
{
    action(frame, 1);
}


/* EN/RU: Broker consumes exactly one explicit Release obligation before wire publication.
 * Broker потребляет ровно один explicit Release до wire publication. */
void Proxygc_consume(void *frame)
{
    action(frame, 2);
}


/* EN/RU: Pop one scalar-only finalizer record on its original broker thread.
 * Снять scalar-only finalizer record с исходного broker thread. */
void Proxygc_next(void *frame)
{
    F_Proxygc_next call;

    struct
    {
        LONG session;    /* Scalar owner session, zero if no pending entry. */
        WORD slot;       /* Scalar slot index, never a managed pointer. */
        LONG generation; /* Scalar slot incarnation. */
    } result = {0};

    DvmRemoteHandle owner;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Proxygc_next, ret), sizeof(*call.ret));
    if (dvm_proxy_lease_pop(connection(call.connection), &owner))
    {
        result.session = owner.session;
        result.slot = owner.slot;
        result.generation = owner.generation;
    }
    dvm_sys_scalar_result(&result, call.ret, sizeof(result));
}


/* EN/RU: Terminal close revokes native metadata; token payloads retain their native owners.
 * Terminal close отзывает native metadata; token payloads держат native owners. */
void Proxygc_close(void *frame)
{
    F_Proxygc_close call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Proxygc_close, ret), sizeof(*call.ret));
    dvm_proxy_lease_queue_close(connection(call.connection));
    WORD result = 0;
    dvm_sys_scalar_result(&result, call.ret, sizeof(result));
}


/* EN/RU: Publish immutable descriptors and generated builtin module before guests start.
 * Опубликовать immutable descriptors/generated module до старта guests. */
void dvm_proxy_gc_initialize(void)
{
    connection_type = dtype(free_connection, sizeof(Connection), NULL, 0);
    lease_type = dtype(free_lease, sizeof(Lease), NULL, 0);
    if (!connection_type || !lease_type)
    {
        error(exNomem);
    }
    builtinmod("$Proxygc", Proxygcmodtab, Proxygcmodlen);
}
