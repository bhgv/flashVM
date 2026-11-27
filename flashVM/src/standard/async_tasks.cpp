/* Copyright (c) 2026 bhgv
 * EN: Own script-visible handles to detached AVM1 workers in the launching context.
 * RU: Хранить объекты задач отделённых потоков AVM1 в контексте запуска.
 */
#include "async_tasks.hpp"
#include "../core/runtime.hpp"
#include "../avm1/action_codes.h"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "flashvm/userdata.h"
#include "flashvm/worker.h"
#include <new>

#define FVM_ASYNC_MAX_TASKS 16U /* Retained or pending-reap tasks per context. */


/* [GC:] EN: Native record shared by the wrapper and the owner reaper, never by worker code.
 * RU: Нативная запись обёртки и очистки владельца, недоступная коду рабочего потока. */
struct FVM_AsyncTask
{
    FVM_Worker *worker;   /* Owned join handle after launch. */
    FVM_AsyncTask *next;  /* Next record in the launching context. */
    bool released;      /* Wrapper was collected and no method can use this record. */
};


/* [GC:] EN: Request cooperative stop without waiting from GC. Free unlaunched records directly.
 * RU: Запросить кооперативную отмену без ожидания в GC. Запись без потока освободить сразу. */
static void release_task(void *userdata)
{
    FVM_AsyncTask *task = static_cast<FVM_AsyncTask *>(userdata);
    if (task->worker)
    {
        task->released = true;
        fvm_worker_stop(task->worker);
    }
    else
    {
        delete task;
    }
}


/* [GC:] EN: Consume released handles once. Return the number of records that remain.
 * RU: Однократно снять освобождённые потоки и вернуть число оставшихся записей. */
unsigned fvm_async_tasks_reap(FVM_Context *context, bool teardown)
{
    unsigned retained = 0;
    FVM_AsyncTask **link = &context->async_tasks;
    while (*link)
    {
        FVM_AsyncTask *task = *link;
        FVM_WorkerState state;
        fvm_worker_state(task->worker, &state);
        if (task->released && (teardown || state == FVM_WORKER_FINISHED))
        {
            fvm_worker_join(task->worker, 0);
            fvm_worker_destroy(task->worker);
            *link = task->next;
            delete task;
        }
        else
        {
            link = &task->next;
            ++retained;
        }
    }
    return retained;
}


/* [GC:] EN: Copy one status snapshot into an owner-heap object in out. No wait or callback.
 * RU: Скопировать снимок состояния в объект кучи владельца в out. Без ожидания и колбэков. */
static FVM_Result poll_task(FVM_Context *context, FVM_Object *, unsigned count,
                            const FVM_Value *, FVM_Value *out, void *userdata)
{
    if (count)
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_AsyncTask *task = static_cast<FVM_AsyncTask *>(userdata);
    FVM_WorkerState state;
    fvm_worker_state(task->worker, &state);
    const bool done = state == FVM_WORKER_FINISHED;
    FVM_Value result;
    fvm_value_undefined(&result);
    FVM_Result completed = done ? fvm_worker_result(task->worker, &result) : FVM_NOT_FOUND;
    if (completed != FVM_OK)
    {
        fvm_value_undefined(&result);
    }
    FVM_Object *snapshot = 0;
    FVM_Result status = fvm_object_create(context, &snapshot);
    if (status != FVM_OK)
    {
        return status;
    }
    status = fvm_gc_pin(context, snapshot);
    if (status != FVM_OK)
    {
        return status;
    }
    FVM_Value field;
    fvm_value_boolean(&field, done);
    status = fvm_object_set(context, snapshot, "done", &field);
    if (status == FVM_OK)
    {
        fvm_value_number(&field, completed);
        status = fvm_object_set(context, snapshot, "status", &field);
    }
    if (status == FVM_OK)
    {
        status = fvm_object_set(context, snapshot, "value", &result);
    }
    if (status == FVM_OK)
    {
        fvm_value_undefined(out);
        out->type = FVM_VALUE_OBJECT;
        out->object = snapshot;
    }
    fvm_gc_unpin(context, snapshot);
    return status;
}


/* EN: Request cooperative cancellation of this task without blocking its caller.
 * RU: Запросить кооперативную остановку задачи без блокировки вызывающего. */
static FVM_Result cancel_task(FVM_Context *, FVM_Object *, unsigned count,
                              const FVM_Value *, FVM_Value *out, void *userdata)
{
    if (count)
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_AsyncTask *task = static_cast<FVM_AsyncTask *>(userdata);
    fvm_value_undefined(out);
    return fvm_worker_stop(task->worker);
}


/* [GC:] EN: Wrap the sole function argument in a task, with no caller heap pointers in the worker.
 * RU: Создать задачу для единственного аргумента-функции без указателей на кучу вызова. */
FVM_Result fvm_builtin_async_run(FVM_Context *context, FVM_Object *, unsigned count,
                                const FVM_Value *args, FVM_Value *out, void *)
{
    if (!context || !out || count != 1 || !args || args[0].type != FVM_VALUE_OBJECT ||
        context->destroying || context->gc_collecting || context->gc_releasing)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (fvm_async_tasks_reap(context, false) >= FVM_ASYNC_MAX_TASKS)
    {
        return FVM_UNSUPPORTED;
    }
    FVM_Object *wrapper = 0;
    FVM_Result status = fvm_object_create(context, &wrapper);
    if (status != FVM_OK)
    {
        return status;
    }
    status = fvm_gc_pin(context, wrapper);
    if (status != FVM_OK)
    {
        return status;
    }
    FVM_AsyncTask *task = new (std::nothrow) FVM_AsyncTask();
    if (!task)
    {
        fvm_gc_unpin(context, wrapper);
        return FVM_OUT_OF_MEMORY;
    }
    status = fvm_userdata_attach(context, wrapper, task, release_task);
    if (status != FVM_OK)
    {
        delete task;
    }
    if (status == FVM_OK)
    {
        status = fvm_object_define_method_rooted(context, wrapper, "poll", poll_task, task, wrapper);
    }
    if (status == FVM_OK)
    {
        status = fvm_object_define_method_rooted(context, wrapper, "cancel", cancel_task, task, wrapper);
    }
    if (status == FVM_OK)
    {
        FVM_Config config = context->runtime->config;
        config.userdata = 0;
        status = fvm_worker_run_function(context, &config, args[0].object, &task->worker);
    }
    if (status == FVM_OK)
    {
        task->next = context->async_tasks;
        context->async_tasks = task;
        fvm_value_undefined(out);
        out->type = FVM_VALUE_OBJECT;
        out->object = wrapper;
    }
    fvm_gc_unpin(context, wrapper);
    return status;
}


/* EN: Wrap no-argument dispatch without forwarding the owner method receiver.
 * RU: Запустить обёрнутую функцию без аргументов и получателя метода владельца. */
static FVM_Result call_detached(FVM_Context *context, FVM_Object *, unsigned count,
                                 const FVM_Value *, FVM_Value *out, void *userdata)
{
    if (count)
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_Value function;
    fvm_value_undefined(&function);
    function.type = FVM_VALUE_OBJECT;
    function.object = static_cast<FVM_Object *>(userdata);
    return fvm_builtin_async_run(context, 0, 1, &function, out, 0);
}


/* [GC:] EN: Prepare a detached worker template from a compiler-supplied
 * zero-argument DefineFunction. Copy only its portable code and constant pool.
 * Keep any captured owner object and timeline in the source context.
 * RU: Подготовить отделённый шаблон из DefineFunction без аргументов,
 * созданного компилятором. Скопировать только переносимый код и константы.
 * Захваченное окружение и шкала остаются в исходном контексте. */
FVM_Result fvm_builtin_async_wrap(FVM_Context *context, FVM_Object *, unsigned count,
                                 const FVM_Value *args, FVM_Value *out, void *)
{
    if (count != 1 || args[0].type != FVM_VALUE_OBJECT || !args[0].object)
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_Object *function = args[0].object;
    if (function->resource_kind != FVM_RESOURCE_SCRIPT_FUNCTION ||
        function->script_body.callback || !function->resource_result_valid ||
        function->resource_result.type != FVM_VALUE_OBJECT)
    {
        return FVM_UNSUPPORTED;
    }
    FVM_Value length;
    FVM_Result status = fvm_object_get(context, function, "length", &length);
    if (status != FVM_OK || length.type != FVM_VALUE_NUMBER || length.number)
    {
        return status == FVM_OK ? FVM_UNSUPPORTED : status;
    }

    FVM_Object *source = function->resource_result.object;
    int function2 = 0;
    status = fvm_object_has_own(context, source, "function2", &function2);
    if (status != FVM_OK || function2)
    {
        return status == FVM_OK ? FVM_UNSUPPORTED : status;
    }

    const unsigned char empty_body[] = {FVM_AVM1_ACTION_END};
    FVM_Object *detached = 0;
    status = fvm_function_create_avm1(context, empty_body, sizeof(empty_body), 0, 0, 0, &detached);
    if (status != FVM_OK)
    {
        return status;
    }
    status = fvm_gc_pin(context, detached);
    if (status != FVM_OK)
    {
        return status;
    }
    FVM_Object *target = detached->resource_result.object;
    static const char *const fields[] = {"code", "constants"};
    for (unsigned i = 0; status == FVM_OK && i < sizeof(fields) / sizeof(*fields); ++i)
    {
        FVM_Value field;
        status = fvm_object_get(context, source, fields[i], &field);
        if (status == FVM_OK)
        {
            status = fvm_object_set(context, target, fields[i], &field);
        }
    }
    if (status == FVM_OK)
    {
        FVM_Value no_timeline;
        fvm_value_undefined(&no_timeline);
        no_timeline.type = FVM_VALUE_NULL;
        status = fvm_object_set(context, target, "base_clip", &no_timeline);
    }
    if (status == FVM_OK)
    {
        FVM_Object *wrapper = 0;
        status = fvm_function_create_native(context, call_detached, detached, detached, &wrapper);
        if (status == FVM_OK)
        {
            fvm_value_undefined(out);
            out->type = FVM_VALUE_OBJECT;
            out->object = wrapper;
        }
    }
    fvm_gc_unpin(context, detached);
    return status;
}
