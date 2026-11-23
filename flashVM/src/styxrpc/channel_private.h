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
    struct StyxClient *reverse;        /* Borrowed reverse mount during a callback. */
    StyxRpcChannelCall *call;          /* Mutable copy of the scoped reverse grants. */
    size_t references;                 /* Owner-local count including each live wrapper. */
    unsigned wait_ms;                  /* Per-operation polling budget. */
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


/* [GC:] EN: Release a script-local typed queue after the owning object retires.
 * RU: Освободить локальную типизированную очередь при уничтожении объекта. */
void fvm_styxrpc_local_channel_release(void *state);


/* EN: Borrow typed userdata only from an object of this owner.
 * RU: Заимствовать userdata правильного типа только у объекта этого владельца. */
void *fvm_styxrpc_local_channel_lookup(FVM_Context *context, FVM_Object *object);


/* EN: Borrow the queue of an open script-local channel.
 * RU: Заимствовать очередь открытого локального канала. */
StyxRpcChannelQueue *fvm_styxrpc_local_channel_queue(FVM_Context *context, const FVM_Value *value);

#ifdef __cplusplus
}
#endif
#endif
