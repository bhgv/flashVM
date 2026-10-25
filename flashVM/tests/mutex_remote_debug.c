#include <stdio.h>
#include <stdlib.h>
#include "flashvm/capability.h"
#include "flashvm/debug.h"
#include "flashvm/exception.h"
#include "flashvm/mutex.h"
#include "flashvm/worker.h"
#include "flashvm/serialization.h"
#include "../ports/thread_port.h"
#include <string.h>

#ifdef OS_PORTS
typedef struct Task
{
    FVM_ExceptionInfo fault;
    unsigned long context;
    int has_fault;
} Task;


static FVM_Result acquire(FVM_Context *c, const FVM_Value *in, FVM_Value *out, void *raw)
{
    Task *task = (Task *)raw;
    FVM_Object *proxy;
    FVM_Result r;
    task->context = fvm_context_identity(c);
    if (!(fvm_debug_set_location(c, "remote-wait.as", 27) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_broker_import(c, in->string, &proxy) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    r = fvm_object_call(c, proxy, "set", 0, 0, out);
    if (r == FVM_OK)
    {
        if (!(fvm_object_call(c, proxy, "reset", 0, 0, out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else if (r == FVM_EXCEPTION)
    {
        if (!(fvm_exception_info(c, &task->fault) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        task->has_fault = 1;
    }
    return r;
}


static void wait_visible(const char *uri)
{
    FVM_DebugMutexDetail rows[16];
    size_t count, i;
    int attempt;
    for (attempt = 0; attempt < 5000; ++attempt)
    {
        if (!(fvm_debug_mutex_detail_snapshot(rows, 16, &count) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (i = 0; i < count; ++i)
        {
            if (rows[i].remote && rows[i].state.waiter && !strcmp(rows[i].address, uri))
            {
                if (!(rows[i].state.line == 27 && !strcmp(rows[i].state.source, "remote-wait.as")))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(!rows[i].state.deadlocked && !rows[i].state.owner))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                return;
            }
        }
        fvm_port_thread_sleep_us(1000);
    }
    if (!(0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void no_waiters(void)
{
    FVM_DebugMutexDetail rows[16];
    size_t count, i;
    if (!(fvm_debug_mutex_detail_snapshot(rows, 16, &count) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < count; ++i)
    {
        if (!(!rows[i].state.waiter))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
}
#endif


int main(void)
{
    size_t count = 0;
    if (!(fvm_debug_mutex_detail_snapshot(0, 1, &count) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_debug_mutex_detail_snapshot(0, 0, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#ifdef OS_PORTS
    {
        FVM_Config cfg;
        FVM_Runtime *rt;
        FVM_Context *owner;
        FVM_ResourceBroker *broker;
        FVM_Mutex *mutex;
        FVM_Object *wrapper;
        FVM_Worker *worker;
        FVM_Value in, out;
        FVM_DebugMutexDetail sentinel, unchanged;
        char *uri;
        Task task;
        int mode, held;
        if (!(fvm_config_init(&cfg) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_context_create(rt, &owner) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_broker_start("127.0.0.1", "127.0.0.1", 0, &broker) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_context_set_broker(owner, broker) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_mutex_create(owner, &mutex) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_extensions_wrap_mutex(owner, mutex, &wrapper) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_broker_export(owner, wrapper, &uri) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_value_string(&in, uri) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (mode = 0; mode < 3; ++mode)
        {
            if (!(fvm_debug_set_location(owner, "authority.as", 12) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_mutex_set(owner, mutex, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            memset(&task, 0, sizeof(task));
            if (!(fvm_worker_run(&cfg, acquire, &in, &task, &worker) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            wait_visible(uri);
            memset(&sentinel, 0x5a, sizeof(sentinel));
            unchanged = sentinel;
            if (!(fvm_debug_mutex_detail_snapshot(&sentinel, 1, &count) == FVM_NOT_FOUND && count >= 2))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(!memcmp(&sentinel, &unchanged, sizeof(sentinel))))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (mode == 0)
            {
                if (!(fvm_mutex_reset(owner, mutex) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else if (mode == 1)
            {
                if (!(fvm_worker_stop(worker) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!(fvm_mutex_close(mutex) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(fvm_worker_join(worker, &out) == (mode ? FVM_EXCEPTION : FVM_OK)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (mode)
            {
                if (!(task.has_fault && task.fault.code == (mode == 1 ? FVM_RESOURCE_CANCELLED : FVM_RESOURCE_CLOSED)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(task.fault.line == 27 && !strcmp(task.fault.source, "remote-wait.as")))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(task.fault.thread == task.context && !strcmp(task.fault.address, uri)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            fvm_worker_destroy(worker);
            no_waiters();
            if (mode == 1)
            {
                if (!(fvm_mutex_reset(owner, mutex) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (mode < 2)
            {
                if (!(fvm_mutex_touch(mutex, &held) == FVM_OK && !held))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        fvm_mutex_release(mutex);
        fvm_broker_stop(broker);
        fvm_broker_release(broker);
        fvm_context_destroy(owner);
        fvm_runtime_destroy(rt);
        fvm_serialized_free(uri);
    }
#endif
    if (!(fvm_debug_mutex_detail_snapshot(0, 0, &count) == FVM_OK && !count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
