/* Copyright (c) 2026 bhgv
 * EN: Internal owner state for one script channel capability.
 * RU: Внутренние данные владельца отдельного канала сценария.
 */
#ifndef FVM_STYXRPC_CHANNEL_PRIVATE_H
#define FVM_STYXRPC_CHANNEL_PRIVATE_H
#include "flashvm/styxrpc_channel.h"
#include "flashvm/gc_roots.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FVM_StyxChannelSet
{
    StyxRpcChannelLease *lease;        /* One retained result lease, if any. */
    struct FVM_StyxLocalLoan *loans;   /* Reverse publications owned by this family. */
    struct StyxClient *reverse;        /* Borrowed reverse mount during a callback. */
    StyxRpcChannelCall *call;          /* Mutable copy of the scoped reverse grants. */
    size_t references;                 /* Owner-local count including each live wrapper. */
    unsigned wait_ms;                  /* Per-operation polling budget. */
    unsigned staging;                  /* Wrapper slots under construction. */
    FVM_Object *objects[STYXRPC_CHANNEL_SLOTS]; /* Weak aliases cleared by close or GC. */
} FVM_StyxChannelSet;

typedef struct FVM_StyxRemoteChannel
{
    FVM_StyxChannelSet *set;    /* Retained family until close or GC. */
    StyxRpcChannelGrant grant; /* Exact key and element type from the wire. */
    size_t index;              /* Stable slot in the family's grant table. */
} FVM_StyxRemoteChannel;

typedef struct FVM_StyxChannelDecode
{
    FVM_StyxChannelSet *set; /* Borrowed checked scope or result set. */
    size_t parent;           /* Present route index for a leased element. */
    int exported;            /* Decode module export tokens in an initial result. */
    int delegated;           /* Decode authenticated module grants from a reverse message. */
} FVM_StyxChannelDecode;


/* EN: Create one owned family for a retained lease or a borrowed reverse call.
 * RU: Создать семейство для удерживаемой аренды или заимствованного встречного вызова. */
FVM_StyxChannelSet *fvm_styxrpc_channel_set_create(StyxRpcChannelLease *lease,
    struct StyxClient *reverse, StyxRpcChannelCall *call, unsigned wait_ms);


/* EN: Release a family reference and its lease on last release, deferring I/O under GC.
 * RU: Снять ссылку семейства и освободить аренду при последней ссылке, отложив I/O под GC. */
void fvm_styxrpc_channel_set_release(FVM_StyxChannelSet *set, int deferred);


/* EN: Close every wrapper while retaining the owner's reference to set.
 * RU: Закрыть все обёртки, удерживая ссылку владельца на set. */
void fvm_styxrpc_channel_set_disarm(FVM_Context *context, FVM_StyxChannelSet *set);


/* [GC:] EN: Find or allocate the wrapper for a checked grant and root it in roots.
 * RU: Найти или выделить обёртку проверенного полномочия и удержать её в roots. */
int fvm_styxrpc_channel_materialize(FVM_Context *context, FVM_GCRootScope *roots,
    FVM_StyxChannelSet *set, size_t index, const StyxRpcChannelGrant *grant, FVM_Value *value);


/* [GC:] EN: Decode a prevalidated leaf against its scoped family's already admitted keys.
 * RU: Декодировать проверенный лист по уже допущенным ключам его семейства. */
int fvm_styxrpc_channel_decode(FVM_Context *context, FVM_GCRootScope *roots, void *state,
    const char *type, const FVM_SRecord *record, FVM_Value *value);


typedef struct FVM_StyxPidGrant
{
    FVM_StyxChannelSet *set;    /* Retained issuing family and its reverse publications. */
    StyxRpcChannelRoute route;  /* Host-authorized reverse endpoint. */
} FVM_StyxPidGrant;

typedef struct FVM_StyxChannelSend
{
    const FVM_StyxRemoteChannel *receiver;  /* Exact sending method receiver. */
    StyxRpcChannelRoutes delegated;         /* Distinct PID routes offered in this message. */
} FVM_StyxChannelSend;


/* EN: Authorize an offered PID or exact current-token key before sending.
 * RU: Разрешить предложенный PID либо точный ключ текущего токена перед отправкой. */
int fvm_styxrpc_channel_send_allow(const void *context, const StyxRpcChannelRoute *route);


/* EN: Borrow a host-created PID grant from the exact context.
 * RU: Заимствовать созданное хостом полномочие PID из точного контекста. */
FVM_StyxPidGrant *fvm_styxrpc_pid_lookup(FVM_Context *context, FVM_Object *object);


/* EN: Borrow the result family of an open anchor in context, or return NULL.
 * RU: Заимствовать семейство результата открытого anchor в context либо вернуть NULL. */
FVM_StyxChannelSet *fvm_styxrpc_channel_lease_set(FVM_Context *context, const FVM_Value *anchor);


/* [GC:] EN: Make an opaque value and retain set. Caller keeps set reachable and fills
 * payload's route before the value escapes. Both outputs are required.
 * RU: Создать непрозрачное значение и удержать set. Вызывающий сохраняет достижимость
 * set и заполняет маршрут payload до выдачи значения. Оба выхода обязательны. */
FVM_Result fvm_styxrpc_channel_create_pid(FVM_Context *context, FVM_StyxChannelSet *set,
    FVM_StyxPidGrant **payload, FVM_Value *value);


/* [GC:] EN: Release a PID message value and retire its native lease reference.
 * RU: Освободить значение PID и снять его ссылку на нативную аренду. */
void fvm_styxrpc_pid_release(void *state);


/* [GC:] EN: Retire all local publications of set before dropping its remote lease.
 * RU: Отозвать все локальные публикации set до освобождения удалённой аренды. */
void fvm_styxrpc_channel_retire_loans(FVM_StyxChannelSet *set);


/* [GC:] EN: Encode a nested null or live channel on the target's exact current bus.
 * RU: Закодировать вложенный null либо живой канал на точной шине назначения. */
int fvm_styxrpc_channel_encode(FVM_Context *context, FVM_GCRootScope *roots, void *state,
    const char *type, const FVM_Value *value, unsigned char *bytes, size_t capacity, size_t *used);


/* [GC:] EN: Release the native payload from the typed userdata slot.
 * RU: Освободить данные из типизированного слота userdata. */
void fvm_styxrpc_channel_release(void *state);


/* EN: Close a root-local endpoint once, optionally deferring the last remote clunk.
 * RU: Закрыть локальный канал один раз, при необходимости отложив последний clunk. */
void fvm_styxrpc_channel_disarm(FVM_StyxRemoteChannel *channel, int deferred);


/* EN: Borrow only a correctly typed endpoint owned by this context.
 * RU: Заимствовать только правильно типизированный канал этого контекста. */
FVM_StyxRemoteChannel *fvm_styxrpc_channel_lookup(FVM_Context *context, FVM_Object *object);


/* [GC:] EN: Encode one element using the legacy int or managed-element framing.
 * RU: Закодировать элемент обычным кадром int либо управляемым кадром элемента. */
int fvm_styxrpc_channel_pack(FVM_Context *context, FVM_GCRootScope *roots, const char *shape,
    const FVM_Value *value, unsigned char *payload, unsigned char *element, StyxRpcGuard *guard);


struct FVM_StyxRefs; /* Owner-local typed graph hooks. */


/* [GC:] EN: Encode one element using the supplied typed graph hooks for nested leaves.
 * RU: Кодировать элемент с указанными хуками графа для вложенных листьев. */
int fvm_styxrpc_channel_pack_refs(FVM_Context *context, FVM_GCRootScope *roots,
    struct FVM_StyxRefs *refs, const char *shape, const FVM_Value *value,
    unsigned char *payload, unsigned char *element, StyxRpcGuard *guard);


/* [GC:] EN: Decode the exact channel element or a successful idle selection.
 * RU: Декодировать точный элемент канала либо подтверждённую пустую ветку. */
int fvm_styxrpc_channel_unpack(FVM_Context *context, FVM_GCRootScope *roots, const char *shape,
    const StyxRpcChannelResult *result, FVM_Value *out);


/* EN: Check a validated channel signature for exactly one result channel.
 * RU: Проверить, что поддерживаемая сигнатура возвращает ровно один канал. */
int fvm_styxrpc_single_channel_result(const StyxRpcSchema *schema);


/* [GC:] EN: Create and seed a portable result resource under roots, transferring
 * it to output on success. The resource contains only copied data.
 * RU: Создать переносимый ресурс под корнями и передать его в output при успехе.
 * Ресурс содержит только копируемые данные. */
int fvm_styxrpc_channel_output(FVM_Context *context, FVM_GCRootScope *roots,
    const StyxRpcSchema *schema, unsigned capacity, const FVM_Value *seed, StyxRpcChannelOutput *output);


typedef struct FVM_StyxLocalRefs
{
    uint32_t keys[STYXRPC_CHANNEL_SLOTS]; /* Slot-local keys, unused entries are zero. */
    FVM_Object *objects[STYXRPC_CHANNEL_SLOTS]; /* Borrowed from the traced queue storage. */
    int issued; /* Use native capacity markers on frames returned from PID publication. */
} FVM_StyxLocalRefs;


/* [GC:] EN: Allocate a hidden, pre-sized array tracing every queued channel reference.
 * RU: Выделить скрытый массив фиксированного размера для трассировки ссылок в очереди. */
FVM_Result fvm_styxrpc_local_edges_init(FVM_Context *context, FVM_Object *owner, unsigned capacity);


/* EN: Get or update a preallocated traced reference without allocating or invoking AS.
 * RU: Прочитать либо заменить заранее выделенную ссылку без выделения памяти и вызова AS. */
FVM_Object *fvm_styxrpc_local_edge(FVM_Object *owner, size_t slot, size_t index, FVM_Object *value, int store);


/* [GC:] EN: Borrow a member of the hidden graph array using its one-based wire key.
 * RU: Заимствовать участника скрытого массива графа по сетевому ключу с единицы. */
FVM_Object *fvm_styxrpc_local_graph_node(FVM_Object *graph, uint32_t key);


/* EN: Get or replace a physical slot's exact key-to-object map. No heap work.
 * RU: Получить либо заменить соответствия ключей и объектов физического слота без работы с кучей. */
void fvm_styxrpc_local_slot(FVM_Context *context, FVM_Object *owner, size_t slot, FVM_StyxLocalRefs *refs, int store);


/* [GC:] EN: Decode an element using the supplied hooks and root decoded values.
 * RU: Декодировать и удержать элемент через указанные хуки графа. */
int fvm_styxrpc_channel_unpack_refs(FVM_Context *context, FVM_GCRootScope *roots,
    struct FVM_StyxRefs *refs, const char *shape, const StyxRpcChannelResult *result, FVM_Value *out);


/* [GC:] EN: Release a script-local typed queue after the owning object retires.
 * RU: Освободить локальную типизированную очередь при уничтожении объекта. */
void fvm_styxrpc_local_channel_release(void *state);


/* EN: Borrow typed userdata only from an object of this owner.
 * RU: Заимствовать userdata правильного типа только у объекта этого владельца. */
void *fvm_styxrpc_local_channel_lookup(FVM_Context *context, FVM_Object *object);


/* EN: Borrow the queue of an open script-local channel.
 * RU: Заимствовать очередь открытого локального канала. */
StyxRpcChannelQueue *fvm_styxrpc_local_channel_queue(FVM_Context *context, const FVM_Value *value);


/* EN: Pin a queue from value and lock out local use on its owner.
 * Set owner to the native pin on success, otherwise return NULL.
 * RU: Удержать очередь из value и запретить параллельное использование.
 * При успехе записать в owner удерживаемое состояние, иначе вернуть NULL. */
StyxRpcChannelQueue *fvm_styxrpc_local_channel_borrow(FVM_Context *context, const FVM_Value *value, void **owner);


/* [GC:] EN: Unlock and release the native pin. If issued is set, resolve returned
 * nested slots on the next owner access, using the already traced graph.
 * RU: Разблокировать и снять нативную ссылку. Если задан issued, восстановить
 * возвращённые вложенные слоты при следующем доступе через трассируемый граф. */
void fvm_styxrpc_local_channel_return(void *owner, int issued);

#ifdef __cplusplus
}
#endif
#endif
