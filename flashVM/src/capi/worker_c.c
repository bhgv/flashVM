/* FlashVM: C embedding API: worker.
 * File: src/capi/worker_c.c.
 */

#include "flashvm/worker.h"
#include "../core/worker_context.h"
#include "../../ports/thread_port.h"
#include "../../ports/channel_sync.h"
#include <stdlib.h>
#include <string.h>


/* Define FVM_Worker struct and its representation. */
struct FVM_Worker
{
    FVM_PortThread *thread;
    FVM_Config config;
    FVM_WorkerFunction callback;
    void *userdata;
    FVM_Value input;
    FVM_Value output;
    FVM_Result result;
    int joined;
    int joining;
    FVM_PortSync *sync;
    int started;
    int finished;
    int stop_requested;
};


/* Copy value. */
static FVM_Result copy_value(FVM_Value *dst, const FVM_Value *src)
{

    /* Initialization / Инициализация локального состояния. */
    size_t len;
    char *s;
    if (!dst || !src || src->type < FVM_VALUE_UNDEFINED || src->type >= FVM_VALUE_OBJECT)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (src->metatable)
    {
        return FVM_UNSUPPORTED;
    }
    *dst = *src;
    if (src->type != FVM_VALUE_STRING)
    {
        return FVM_OK;
    }
    if (!src->string)
    {
        return FVM_INVALID_ARGUMENT;
    }

    /* Computation / Вычисление и обновление состояния. */
    len = fvm_value_string_size(src);
    s = (char *)malloc(len + 1);
    if (!s)
    {
        return FVM_OUT_OF_MEMORY;
    }

    /* Computation / Вычисление и обновление состояния. */
    memcpy(s, src->string, len);
    s[len] = 0;
    dst->string = s;
    return FVM_OK;
}


/* Implement free value for this subsystem. */
static void free_value(FVM_Value *v)
{
    if (v->type == FVM_VALUE_STRING)
    {
        free((void *)v->string);
    }
    v->string = 0;
}


/* Implement worker entry for this subsystem. */
static void worker_entry(void *arg)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Worker *w = (FVM_Worker *)arg;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    FVM_Value result;
    FVM_Result r;
    fvm_port_sync_lock(w->sync);
    while (!w->started)
    {
        fvm_port_sync_wait(w->sync);
    }
    fvm_port_sync_unlock(w->sync);


    /* Initialization / Инициализация локального состояния. */
    fvm_value_undefined(&result);
    r = fvm_runtime_create(&w->config, &runtime);
    if (r == FVM_OK)
    {
        r = fvm_context_create(runtime, &context);
    }
    if (r == FVM_OK)
    {
        fvm_context_bind_worker(context, w);
        r = w->callback(context, &w->input, &result, w->userdata);
        if (r == FVM_OK)
        {
            r = copy_value(&w->output, &result);
        }
    }
    if (context)
    {
        fvm_context_destroy(context);
    }
    if (runtime)
    {
        fvm_runtime_destroy(runtime);
    }

    /* Computation / Вычисление и обновление состояния. */
    fvm_port_sync_lock(w->sync);
    w->result = r;
    w->finished = 1;
    fvm_port_sync_signal_all(w->sync);
    fvm_port_sync_unlock(w->sync);
}


/* Create worker. */
static FVM_Result worker_create(const FVM_Config *config,
                                FVM_WorkerFunction callback,
                                const FVM_Value *input,
                                void *userdata,
                                FVM_Worker **out,
                                int start_now)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Worker *w;
    FVM_Result r;
    if (!config || !callback || !input || !out || config->struct_size != sizeof(FVM_Config))
    {
        return FVM_INVALID_ARGUMENT;
    }

    /* Computation / Вычисление и обновление состояния. */
    *out = 0;
    w = (FVM_Worker *)calloc(1, sizeof(*w));
    if (!w)
    {
        return FVM_OUT_OF_MEMORY;
    }

    /* Computation / Вычисление и обновление состояния. */
    w->config = *config;
    w->callback = callback;
    w->userdata = userdata;
    w->result = FVM_ERROR;
    w->started = 0;
    w->sync = fvm_port_sync_create();
    if (!w->sync)
    {
        free(w);
        return FVM_OUT_OF_MEMORY;
    }
    fvm_value_undefined(&w->output);
    r = copy_value(&w->input, input);
    if (r != FVM_OK)
    {
        fvm_port_sync_destroy(w->sync);
        free(w);
        return r;
    }
    w->thread = fvm_port_thread_start(worker_entry, w);
    if (!w->thread)
    {
        free_value(&w->input);
        fvm_port_sync_destroy(w->sync);
        free(w);
        return FVM_ERROR;
    }

    /* Computation / Вычисление и обновление состояния. */
    *out = w;
    /* Publish the OS handle before callbacks can inspect or join themselves. */
    fvm_port_sync_lock(w->sync);
    w->started = start_now;
    if (start_now)
    {
        fvm_port_sync_signal_all(w->sync);
    }
    fvm_port_sync_unlock(w->sync);
    return FVM_OK;
}


/* Run worker. */
FVM_Result fvm_worker_run(const FVM_Config *c, FVM_WorkerFunction f, const FVM_Value *v, void *ud, FVM_Worker **out)
{
    return worker_create(c, f, v, ud, out, 1);
}


/* Run worker suspended. */
FVM_Result
fvm_worker_run_suspended(const FVM_Config *c, FVM_WorkerFunction f, const FVM_Value *v, void *ud, FVM_Worker **out)
{
    return worker_create(c, f, v, ud, out, 0);
}


/* Compute and return worker start. */
FVM_Result fvm_worker_start(FVM_Worker *w)
{
    if (!w)
    {
        return FVM_INVALID_ARGUMENT;
    }

    /* Computation / Вычисление и обновление состояния. */
    fvm_port_sync_lock(w->sync);
    w->started = 1;
    fvm_port_sync_signal_all(w->sync);
    fvm_port_sync_unlock(w->sync);
    return FVM_OK;
}


/* Compute and return worker state. */
FVM_Result fvm_worker_state(FVM_Worker *w, FVM_WorkerState *state)
{
    if (!w || !state)
    {
        return FVM_INVALID_ARGUMENT;
    }

    /* Computation / Вычисление и обновление состояния. */
    fvm_port_sync_lock(w->sync);
    *state = w->joined     ? FVM_WORKER_JOINED
             : w->finished ? FVM_WORKER_FINISHED
             : w->started  ? FVM_WORKER_RUNNING
                           : FVM_WORKER_SUSPENDED;
    fvm_port_sync_unlock(w->sync);
    return FVM_OK;
}


/* Join worker. */
FVM_Result fvm_worker_join(FVM_Worker *w, FVM_Value *out)
{
    FVM_PortThread *thread;
    if (!w)
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_port_sync_lock(w->sync);
    if (!w->finished && w->thread && fvm_port_thread_is_current(w->thread))
    {
        fvm_port_sync_unlock(w->sync);
        return FVM_INVALID_ARGUMENT;
    }
    while (w->joining && !w->joined)
    {
        fvm_port_sync_wait(w->sync);
    }
    if (!w->joined)
    {
        w->joining = 1;
        w->started = 1;
        thread = w->thread;
        fvm_port_sync_signal_all(w->sync);
        fvm_port_sync_unlock(w->sync);
        fvm_port_thread_join(thread);
        fvm_port_sync_lock(w->sync);
        w->thread = 0;
        w->joined = 1;
        w->joining = 0;
        fvm_port_sync_signal_all(w->sync);
    }
    if (out)
    {
        *out = w->output;
    }
    fvm_port_sync_unlock(w->sync);
    return w->result;
}


/* Destroy worker. */
void fvm_worker_destroy(FVM_Worker *w)
{
    if (!w || !w->joined)
    {
        return;
    }

    /* Computation / Вычисление и обновление состояния. */
    free_value(&w->input);
    free_value(&w->output);

    /* Cleanup / Освобождение ресурсов. */
    fvm_port_sync_destroy(w->sync);
    free(w);
}


/* Compute and return worker stop. */
FVM_Result fvm_worker_stop(FVM_Worker *w)
{
    if (!w)
    {
        return FVM_INVALID_ARGUMENT;
    }

    /* Computation / Вычисление и обновление состояния. */
    fvm_port_sync_lock(w->sync);
    w->stop_requested = 1;
    w->started = 1;
    fvm_port_sync_signal_all(w->sync);
    fvm_port_sync_unlock(w->sync);
    return FVM_OK;
}


/* Compute and return worker stop requested. */
FVM_Result fvm_worker_stop_requested(FVM_Worker *w, int *requested)
{
    if (!w || !requested)
    {
        return FVM_INVALID_ARGUMENT;
    }

    /* Computation / Вычисление и обновление состояния. */
    fvm_port_sync_lock(w->sync);
    *requested = w->stop_requested;
    fvm_port_sync_unlock(w->sync);
    return FVM_OK;
}


/* Join worker all. */
FVM_Result fvm_worker_join_all(FVM_Worker **workers, unsigned int count, FVM_Value *results)
{

    /* Initialization / Инициализация локального состояния. */
    unsigned int i, j;
    FVM_Result first = FVM_OK, r;
    if (count && !workers)
    {
        return FVM_INVALID_ARGUMENT;
    }
    for (i = 0; i < count; ++i)
    {
        if (!workers[i])
        {
            return FVM_INVALID_ARGUMENT;
        }
        for (j = 0; j < i; ++j)
        {
            if (workers[i] == workers[j])
            {
                return FVM_INVALID_ARGUMENT;
            }
        }
    }
    for (i = 0; i < count; ++i)
    {
        r = fvm_worker_join(workers[i], results ? &results[i] : 0);
        if (first == FVM_OK && r != FVM_OK)
        {
            first = r;
        }
    }
    return first;
}


/* Compute and return worker result. */
FVM_Result fvm_worker_result(FVM_Worker *w, FVM_Value *out)
{
    FVM_Result r;
    if (!w || !out)
    {
        return FVM_INVALID_ARGUMENT;
    }


    /* Computation / Вычисление и обновление состояния. */
    fvm_port_sync_lock(w->sync);
    r = w->finished ? w->result : FVM_NOT_FOUND;
    if (w->finished)
    {
        *out = w->output;
    }
    fvm_port_sync_unlock(w->sync);
    return r;
}
