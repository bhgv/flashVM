/* Copyright (c) 2026 bhgv
 * EN: Ordinary mounted file access in detached playback tasks.
 * RU: Доступ отделённых задач плеера к обычным смонтированным файлам.
 */
#ifndef FVM_STYXRPC_WORKER_VFS_H
#define FVM_STYXRPC_WORKER_VFS_H
#include "flashvm/value.h"
#include "../standard/async_worker.h"
#include "styxrpc/channel_arguments.h"


/* EN: Hold a PID and its private argument bus for one active task.
 * RU: Удерживать PID и его шину аргументов за одной активной задачей. */
typedef struct FVM_StyxRpcWorkerScope
{
    StyxRpcArgumentBus *bus; /* Owned by the player until all fids and tasks retire. */
    uint32_t pid;           /* Immutable reverse PID matching bus. */
    int in_use;             /* Protected by the platform registry mutex. */
} FVM_StyxRpcWorkerScope;


/* EN: Keep caller identities disjoint for the lifetime of a paired player.
 * RU: Разделить контексты вызывающих на время работы парного плеера. */
typedef struct FVM_StyxRpcWorkerScopes
{
    FVM_StyxRpcWorkerScope entries[FVM_ASYNC_MAX_TASKS]; /* One slot per admitted task. */
} FVM_StyxRpcWorkerScopes;


/* [GC:] EN: Install vfs and its os alias in worker. state is the borrowed
 * playback record. Borrow immutable clients and reserve one synchronized caller scope.
 * RU: Установить vfs и зеркало os в worker. state — заимствованная запись
 * плеера. Заимствовать постоянные клиенты и занять один синхронизированный контекст вызова. */
FVM_Result fvm_styxrpc_worker_vfs(FVM_Context *worker, void *state);

#endif
