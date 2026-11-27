/* Copyright (c) 2026 bhgv
 * EN: Internal script entry and cleanup for detached OS-thread tasks.
 * RU: Внутренний запуск и очистка задач в отдельных системных потоках.
 */
#ifndef FVM_STANDARD_ASYNC_TASKS_HPP
#define FVM_STANDARD_ASYNC_TASKS_HPP /* Guard private declarations. */

#include "flashvm/value.h"


/* EN: Run args[0] on an independent OS thread. Return a task in out.
 * RU: Запустить args[0] в отдельном системном потоке. Вернуть задачу в out. */
FVM_Result fvm_builtin_async_run(FVM_Context *context, FVM_Object *self, unsigned count,
                                const FVM_Value *args, FVM_Value *out, void *unused);


/* [GC:] EN: Capture args[0] for invocation from a compiled worker entry.
 * RU: Захватить args[0] для вызова из входа рабочей функции. */
FVM_Result fvm_builtin_async_wrap(FVM_Context *context, FVM_Object *self, unsigned count,
                                 const FVM_Value *args, FVM_Value *out, void *unused);


/* [GC:] EN: Reclaim released tasks on the owner. Join unfinished ones only at teardown.
 * RU: Освободить снятые задачи у владельца. Ждать незавершённые только при закрытии. */
unsigned fvm_async_tasks_reap(FVM_Context *context, bool teardown);
#endif
