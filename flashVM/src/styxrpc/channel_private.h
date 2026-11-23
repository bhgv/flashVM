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

typedef struct FVM_StyxBorrowedChannels
{
    struct StyxClient *reverse;      /* Same-session client while the callback is active. */
    const StyxRpcChannelCall *call;  /* Checked reverse PID and exact grants. */
} FVM_StyxBorrowedChannels;

typedef struct FVM_StyxRemoteChannel
{
    StyxRpcChannelLease *lease;          /* Owned result lease until close or GC. */
    FVM_StyxBorrowedChannels *borrowed; /* Callback-owned scope, cleared before return. */
    StyxRpcChannelGrant grant; /* Exact key and element type from the reply. */
    size_t index;              /* Its route's position within the lease. */
    unsigned wait_ms;          /* Per-operation polling budget copied at return. */
} FVM_StyxRemoteChannel;


/* [GC:] EN: Create one object for either a retained lease or a scope explicitly disarmed by its owner.
 * RU: Создать объект удерживаемой аренды либо контекста, явно отзываемого владельцем. */
FVM_Result fvm_styxrpc_channel_object(FVM_Context *context, StyxRpcChannelLease *lease,
    FVM_StyxBorrowedChannels *borrowed, const StyxRpcChannelGrant *grant, size_t index,
    unsigned wait_ms, FVM_Value *value);


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
