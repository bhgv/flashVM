/* Copyright (c) 2026 bhgv
 * EN: Native state shared by the local channel methods and publication lifecycle.
 * RU: Нативное состояние для методов локального канала и жизненного цикла публикации.
 */
#ifndef FVM_STYXRPC_CHANNEL_LOCAL_H
#define FVM_STYXRPC_CHANNEL_LOCAL_H
#include "channel_private.h"


/* EN: The ActionScript userdata and a reverse publication may each own this arena.
 * RU: Арена принадлежит объекту ActionScript и, при выдаче, встречной публикации. */
typedef struct LocalChannel
{
    StyxRpcChannelQueue queue; /* The local queue, including when published on a reverse PID. */
    size_t sizes[FVM_STYXRPC_CHANNEL_QUEUE_SLOTS]; /* Length of each encoded element. */
    uint32_t keys[FVM_STYXRPC_CHANNEL_QUEUE_SLOTS][STYXRPC_CHANNEL_SLOTS]; /* Typed leaves of each physical slot. */
    unsigned issued;           /* Physical slots using native capacity flags. */
    int reading;               /* A local receive holds the current slot through GC. */
    unsigned writing;          /* Active encoders may run reentrant script getters. */
    int closed;                /* Explicit close invalidates the object and aliases. */
    int restore;               /* A retired PID left graph keys in the queued frames. */
    int lent;                  /* Only the reverse dispatcher may use the arena. */
    unsigned references;       /* Userdata and optional PID publication each own one. */
} LocalChannel;


/* [GC:] EN: Rebuild slot edges for a retired publication while owner is usable.
 * Return 1 on success or when there is nothing to restore, 0 on an invalid frame.
 * RU: Восстановить ссылки слотов после отзыва публикации у действующего owner.
 * Вернуть 1 при успехе или отсутствии работы, 0 при неверном кадре. */
int fvm_styxrpc_local_sync(FVM_Context *context, FVM_Object *owner, LocalChannel *local);

#endif
