/* Copyright (c) 2026 bhgv
 * EN: Immediate operation for a synchronized mailbox owner.
 * RU: Немедленная операция владельца синхронизированной очереди.
 */
#ifndef STYXRPC_CHANNEL_SCHEDULER_INTERNAL_H
#define STYXRPC_CHANNEL_SCHEDULER_INTERNAL_H
#include "styxrpc/channel_scheduler.h"


/* EN: Run a checked stack-local TRY fid under the scheduler owner's lock.
 * Its guards and grants must match the registered queues. The fid never escapes.
 * RU: Выполнить проверенный локальный TRY fid под мьютексом владельца.
 * Ветки и полномочия совпадают с очередями. Fid не сохраняется в планировщике. */
void styxrpc_scheduler_try_owner(StyxRpcChannelScheduler *scheduler, StyxRpcChannelFid *fid);
#endif
