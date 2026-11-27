/* Copyright (c) 2026 bhgv
 * EN: Ordinary mounted file access in detached playback tasks.
 * RU: Доступ отделённых задач плеера к обычным смонтированным файлам.
 */
#ifndef FVM_STYXRPC_WORKER_VFS_H
#define FVM_STYXRPC_WORKER_VFS_H
#include "flashvm/value.h"


/* [GC:] EN: Install vfs and its os alias in worker. state is the borrowed
 * live Styx client which the host keeps until after the worker teardown.
 * RU: Установить vfs и зеркало os в worker. state — заимствованный
 * клиент Styx, который хост удерживает до закрытия задачи. */
FVM_Result fvm_styxrpc_worker_vfs(FVM_Context *worker, void *state);

#endif
