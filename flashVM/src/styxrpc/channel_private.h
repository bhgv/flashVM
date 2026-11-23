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

typedef struct FVM_StyxRemoteChannel
{
    StyxRpcChannelLease *lease; /* Owned reference until close or GC. */
    StyxRpcChannelGrant grant; /* Exact key and element type from the reply. */
    size_t index;              /* Its route's position within the lease. */
    unsigned wait_ms;          /* Per-operation polling budget copied at return. */
} FVM_StyxRemoteChannel;


/* [GC:] EN: Define one channel object and retain the caller's lease on success.
 * RU: Создать один объект канала и при успехе удержать аренду вызова. */
FVM_Result fvm_styxrpc_channel_object(FVM_Context *context, StyxRpcChannelLease *lease,
    const StyxRpcChannelGrant *grant, size_t index, unsigned wait_ms, FVM_Value *value);


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

#ifdef __cplusplus
}
#endif
#endif
