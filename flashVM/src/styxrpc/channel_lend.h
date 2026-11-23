/* Copyright (c) 2026 bhgv
 * EN: Owner-local staging and retirement for queues with nested call grants.
 * RU: Подготовка и отзыв очередей с вложенными полномочиями у владельца.
 */
#ifndef FVM_STYXRPC_CHANNEL_LEND_H
#define FVM_STYXRPC_CHANNEL_LEND_H
#include "channel_private.h"

typedef struct FVM_StyxLentQueue
{
    FVM_Value owner;            /* Rooted for the whole invocation. */
    StyxRpcChannelQueue *local; /* Original queue owned by owner. */
    StyxRpcChannelQueue shadow; /* Nested-message arena for the exact generation, or empty. */
    StyxRpcChannelGrant grant;  /* Reverse key and type for this invocation. */
    size_t sizes[FVM_STYXRPC_CHANNEL_QUEUE_SLOTS]; /* Shadow frame lengths. */
    int nested;                 /* Frames require generation-specific keys. */
} FVM_StyxLentQueue;

typedef struct FVM_StyxLentChannels
{
    StyxRpcArgumentScope *scope; /* Staged reverse scope, consumed by invoke. */
    FVM_StyxLentQueue lanes[STYXRPC_CHANNEL_SLOTS]; /* Direct and discovered descendants. */
    size_t count;                /* Known distinct queues. */
} FVM_StyxLentChannels;


/* [GC:] EN: Root a direct queue, record its generation key and encode the argument.
 * RU: Удержать прямую очередь, записать ключ поколения и закодировать аргумент. */
int fvm_styxrpc_lend_argument(FVM_Context *context, FVM_GCRootScope *roots, void *state,
    const char *type, const FVM_Value *value, unsigned char *bytes, size_t capacity, size_t *used);


/* [GC:] EN: Traverse queued channel references and prepare exact wire frames before sending.
 * RU: Обойти ссылки очередей и подготовить точные кадры до отправки вызова. */
int fvm_styxrpc_lend_prepare(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxLentChannels *channels);


/* EN: Restore nested queue contents from confirmed effects after scope retirement.
 * RU: После отзыва области восстановить содержимое вложенных очередей по итогам операций. */
int fvm_styxrpc_lend_restore(FVM_Context *context, FVM_StyxLentChannels *channels);


/* EN: Free detached per-generation arenas, after all operations have been retired.
 * RU: Освободить отсоединённые арены поколения после отзыва всех операций. */
void fvm_styxrpc_lend_clear(FVM_StyxLentChannels *channels);

#endif
