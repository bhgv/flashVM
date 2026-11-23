/* Copyright (c) 2026 bhgv
 * EN: Owner-driven bounded FIFO and rendezvous scheduler for C channel providers.
 * RU: Управляемый владельцем планировщик очередей и rendezvous для провайдеров C.
 */
#ifndef STYXRPC_CHANNEL_SCHEDULER_H
#define STYXRPC_CHANNEL_SCHEDULER_H
#include "styxrpc/channel_fid.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STYXRPC_CHANNEL_PENDING (2U * STYXRPC_CHANNEL_SLOTS) /* Maximum queued selections. */

typedef struct StyxRpcChannelQueue
{
    StyxRpcChannelGrant grant; /* Exact admitted key and element shape. */
    unsigned char *bytes;      /* Caller-owned arena, capacity times stride bytes. */
    size_t *sizes;             /* Caller-owned array of capacity frame lengths. */
    size_t capacity;           /* Zero selects synchronous rendezvous. */
    size_t stride;             /* Byte size of each queue slot. */
    size_t head;               /* Private FIFO head, initially zero. */
    size_t count;              /* Private FIFO count, initially zero. */
} StyxRpcChannelQueue;

typedef struct StyxRpcSchedulerEntry
{
    StyxRpcChannelFid *fid; /* Borrowed until completion or cancellation. */
    uint64_t deadline;      /* WAIT expiry, zero for other operations. */
} StyxRpcSchedulerEntry;

typedef struct StyxRpcChannelScheduler
{
    StyxRpcChannelQueue *queues; /* Borrowed stable registrations. */
    size_t count;                /* Exact registered queue count. */
    uint64_t (*clock_ms)(void *context); /* Optional monotonic milliseconds, needed for WAIT. */
    void *context;               /* Borrowed clock context. */
    StyxRpcSchedulerEntry pending[STYXRPC_CHANNEL_PENDING]; /* In admission order. */
    size_t used;                 /* Pending fids. */
} StyxRpcChannelScheduler;


/* EN: Bind fresh caller-owned queues and an optional clock; one owner serializes everything.
 * RU: Привязать свежие очереди приложения и необязательные часы. Все операции сериализует владелец. */
int styxrpc_scheduler_init(StyxRpcChannelScheduler *scheduler, StyxRpcChannelQueue *queues,
    size_t count, uint64_t (*clock_ms)(void *), void *context);


/* EN: Use as the admit callback of a fresh channel fid; a completed fid is removed automatically.
 * RU: Использовать как admit нового fid. Завершённый fid снимается автоматически. */
int styxrpc_scheduler_admit(void *context, StyxRpcChannelFid *fid);


/* EN: Use as the cancel callback of a fid, removing only its queued selection.
 * RU: Использовать как cancel fid, снимая только его ожидающую операцию. */
void styxrpc_scheduler_cancel(void *context, StyxRpcChannelFid *fid);


/* EN: Recheck ready alternatives and bounded WAIT deadlines. Does not block.
 * RU: Проверить готовность веток и сроки WAIT без блокировки. */
void styxrpc_scheduler_progress(StyxRpcChannelScheduler *scheduler);

#ifdef __cplusplus
}
#endif
#endif
