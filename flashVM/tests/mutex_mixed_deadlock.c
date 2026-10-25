/* A shared TCP proxy and a local mutex form a locally observable wait cycle. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/capability.h"
#include "flashvm/channel.h"
#include "flashvm/debug.h"
#include "flashvm/exception.h"
#include "flashvm/mutex.h"
#include "flashvm/serialization.h"
#include "flashvm/transfer.h"
#include "flashvm/worker.h"
#include "../ports/thread_port.h"
#include <string.h>

#ifdef OS_PORTS
typedef struct Task
{
    FVM_Mutex *local;
    FVM_Channel *input, *ready, *go;
    int remote_first;
    unsigned long context;
} Task;


static FVM_Result run(FVM_Context *c, const FVM_Value *in, FVM_Value *out, void *raw)
{
    Task *t = (Task *)raw;
    FVM_Value proxy, value;
    FVM_Result result;
    (void)in;
    t->context = fvm_context_identity(c);
    if (!(fvm_channel_receive_value(c, t->input, &proxy, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_debug_set_location(
              c, t->remote_first ? "mixed-remote.as" : "mixed-local.as", t->remote_first ? 21 : 11) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (t->remote_first)
    {
        if (!(fvm_object_call(c, proxy.object, "set", 0, 0, &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(fvm_mutex_set(c, t->local, 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_value_number(&value, 1);
    if (!(fvm_channel_send(t->ready, &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_receive_owned(t->go, &value, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_channel_message_release(&value);
    if (!(fvm_debug_set_location(
              c, t->remote_first ? "mixed-remote.as" : "mixed-local.as", t->remote_first ? 22 : 12) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (t->remote_first)
    {
        result = fvm_mutex_set(c, t->local, 1);
        if (result == FVM_OK)
        {
            if (!(fvm_mutex_reset(c, t->local) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    else
    {
        result = fvm_object_call(c, proxy.object, "set", 0, 0, &value);
        if (result == FVM_OK)
        {
            if (!(fvm_object_call(c, proxy.object, "reset", 0, 0, &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            FVM_ExceptionInfo fault;
            if (!(result == FVM_EXCEPTION))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_exception_info(c, &fault) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fault.code == FVM_RESOURCE_CANCELLED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    /* Context teardown must release the first lock, including remote ownership. */
    fvm_value_undefined(out);
    return result;
}
#endif


int main(void)
{
#ifdef OS_PORTS
    FVM_Config cfg;
    FVM_Runtime *rt;
    FVM_Context *owner, *client;
    FVM_ResourceBroker *broker;
    FVM_Mutex *authority, *local;
    FVM_Object *wrapper, *proxy;
    FVM_Channel *input, *ready, *go;
    FVM_Worker *workers[2];
    FVM_DebugMutexDetail rows[16];
    FVM_Value value;
    Task tasks[2];
    char *uri;
    size_t count, j;
    int i, attempt, cycles, remote_cycles, held, round;
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
    if (!(fvm_context_create(rt, &client) == FVM_OK))
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
    if (!(fvm_mutex_create(owner, &authority) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_mutex_create(owner, &local) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_extensions_wrap_mutex(owner, authority, &wrapper) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_broker_export(owner, wrapper, &uri) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_broker_import(client, uri, &proxy) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_create(&input) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_create(&ready) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_create(&go) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (round = 0; round < 2; ++round)
    {
        for (i = 0; i < 2; ++i)
        {
            fvm_value_undefined(&value);
            value.type = FVM_VALUE_OBJECT;
            value.object = proxy;
            /* Queue transport retains one native proxy shared by both workers. */
            if (!(fvm_channel_send_value(client, input, &value, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            memset(&tasks[i], 0, sizeof(tasks[i]));
            tasks[i].local = local;
            tasks[i].input = input;
            tasks[i].ready = ready;
            tasks[i].go = go;
            tasks[i].remote_first = i;
            fvm_value_undefined(&value);
            if (!(fvm_worker_run(&cfg, run, &value, &tasks[i], &workers[i]) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        for (i = 0; i < 2; ++i)
        {
            if (!(fvm_channel_receive_owned(ready, &value, 1) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_channel_message_release(&value);
        }
        fvm_value_number(&value, 1);
        if (!(fvm_channel_send(go, &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_channel_send(go, &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        cycles = remote_cycles = 0;
        for (attempt = 0; attempt < 5000; ++attempt)
        {
            if (!(fvm_debug_mutex_detail_snapshot(rows, 16, &count) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            cycles = remote_cycles = 0;
            for (j = 0; j < count; ++j)
            {
                if (rows[j].state.deadlocked)
                {
                    FVM_DebugMutex *state = &rows[j].state;
                    ++cycles;
                    if (!(state->waiter == tasks[rows[j].remote ? 0 : 1].context))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(state->owner == tasks[rows[j].remote ? 1 : 0].context))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(state->line == (unsigned long)(rows[j].remote ? 12 : 22)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(state->owner_line == (unsigned long)(rows[j].remote ? 21 : 11)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(!strcmp(state->source, rows[j].remote ? "mixed-local.as" : "mixed-remote.as")))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (rows[j].remote)
                    {
                        ++remote_cycles;
                        if (!(!strcmp(rows[j].address, uri)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                }
            }
            if (cycles == 2 && remote_cycles == 1)
            {
                break;
            }
            fvm_port_thread_sleep_us(1000);
        }
        if (!(cycles == 2 && remote_cycles == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (i = 0; i < 2; ++i)
        {
            if (!(fvm_worker_stop(workers[i]) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        for (i = 0; i < 2; ++i)
        {
            FVM_Result result = fvm_worker_join(workers[i], 0);
            if (!(result == FVM_OK || result == FVM_ERROR || result == FVM_EXCEPTION))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_worker_destroy(workers[i]);
        }
        if (!(fvm_debug_mutex_detail_snapshot(rows, 16, &count) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (j = 0; j < count; ++j)
        {
            if (!(!rows[j].state.waiter && !rows[j].state.owner && !rows[j].state.deadlocked))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_mutex_touch(authority, &held) == FVM_OK && !held))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_mutex_touch(local, &held) == FVM_OK && !held))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_channel_release(input);
    fvm_channel_release(ready);
    fvm_channel_release(go);
    fvm_mutex_release(local);
    fvm_mutex_release(authority);
    fvm_broker_stop(broker);
    fvm_broker_release(broker);
    fvm_context_destroy(client);
    fvm_context_destroy(owner);
    fvm_runtime_destroy(rt);
    fvm_serialized_free(uri);
    if (!(fvm_debug_mutex_detail_snapshot(0, 0, &count) == FVM_OK && !count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}
