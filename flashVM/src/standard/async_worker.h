/* Copyright (c) 2026 bhgv
 * EN: Install host-selected native services in an independent AS task.
 * RU: Установка выбранных хостом служб в отдельной задаче AS.
 */
#ifndef FVM_STANDARD_ASYNC_WORKER_H
#define FVM_STANDARD_ASYNC_WORKER_H
#include "flashvm/worker.h"

#ifdef __cplusplus
extern "C" {
#endif


/* EN: Native-only setup copied before the task starts. Never stores AS values.
 * RU: Нативная настройка, копируемая перед запуском. Значений AS не содержит. */
typedef struct FVM_AsyncWorkerSetup
{
    FVM_Result (*install)(FVM_Context *, void *); /* Runs before deserialization in the worker. */
    void *state;                                 /* Borrowed native state, independent of the owner heap. */
} FVM_AsyncWorkerSetup;


/* EN: Copy setup for future script tasks on this owner. Host keeps setup.state
 * valid and thread-safe until owner teardown joins its tasks.
 * RU: Скопировать настройку для будущих задач владельца. Хост удерживает
 * потокобезопасный setup.state до завершения задач при закрытии владельца. */
void fvm_async_worker_setup(FVM_Context *owner, const FVM_AsyncWorkerSetup *setup);


/* EN: Start from a copied AVM1 call. Borrow setup (if set) until the worker
 * finishes; the worker calls install in its own context before executing.
 * RU: Запустить копию вызова AVM1. Заимствовать setup до конца работы.
 * Поток вызывает install в своём контексте до исполнения. */
FVM_Result fvm_worker_run_prepared(FVM_Context *owner, const FVM_Config *config,
    FVM_Object *function, unsigned int count, const FVM_Value *args, FVM_AsyncWorkerSetup *setup,
    FVM_Worker **out);

#ifdef __cplusplus
}
#endif
#endif
