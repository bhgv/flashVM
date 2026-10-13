/* A shared TCP proxy and a local mutex form a locally observable wait cycle. */
#include "flashvm/capability.h"
#include "flashvm/channel.h"
#include "flashvm/debug.h"
#include "flashvm/exception.h"
#include "flashvm/mutex.h"
#include "flashvm/serialization.h"
#include "flashvm/transfer.h"
#include "flashvm/worker.h"
#include "../ports/thread_port.h"
#include <assert.h>
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
    assert(fvm_channel_receive_value(c, t->input, &proxy, 1) == FVM_OK);
    assert(fvm_debug_set_location(c, t->remote_first ? "mixed-remote.as" : "mixed-local.as",
                                  t->remote_first ? 21 : 11) == FVM_OK);
    if (t->remote_first)
        assert(fvm_object_call(c, proxy.object, "set", 0, 0, &value) == FVM_OK);
    else
        assert(fvm_mutex_set(c, t->local, 1) == FVM_OK);
    fvm_value_number(&value, 1);
    assert(fvm_channel_send(t->ready, &value) == FVM_OK);
    assert(fvm_channel_receive_owned(t->go, &value, 1) == FVM_OK);
    fvm_channel_message_release(&value);
    assert(fvm_debug_set_location(c, t->remote_first ? "mixed-remote.as" : "mixed-local.as",
                                  t->remote_first ? 22 : 12) == FVM_OK);
    if (t->remote_first)
    {
        result = fvm_mutex_set(c, t->local, 1);
        if (result == FVM_OK)
            assert(fvm_mutex_reset(c, t->local) == FVM_OK);
    }
    else
    {
        result = fvm_object_call(c, proxy.object, "set", 0, 0, &value);
        if (result == FVM_OK)
            assert(fvm_object_call(c, proxy.object, "reset", 0, 0, &value) == FVM_OK);
        else
        {
            FVM_ExceptionInfo fault;
            assert(result == FVM_EXCEPTION);
            assert(fvm_exception_info(c, &fault) == FVM_OK);
            assert(fault.code == FVM_RESOURCE_CANCELLED);
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
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &owner) == FVM_OK);
    assert(fvm_context_create(rt, &client) == FVM_OK);
    assert(fvm_broker_start("127.0.0.1", "127.0.0.1", 0, &broker) == FVM_OK);
    assert(fvm_context_set_broker(owner, broker) == FVM_OK);
    assert(fvm_mutex_create(owner, &authority) == FVM_OK);
    assert(fvm_mutex_create(owner, &local) == FVM_OK);
    assert(fvm_extensions_wrap_mutex(owner, authority, &wrapper) == FVM_OK);
    assert(fvm_broker_export(owner, wrapper, &uri) == FVM_OK);
    assert(fvm_broker_import(client, uri, &proxy) == FVM_OK);
    assert(fvm_channel_create(&input) == FVM_OK);
    assert(fvm_channel_create(&ready) == FVM_OK);
    assert(fvm_channel_create(&go) == FVM_OK);
    for (round = 0; round < 2; ++round)
    {
        for (i = 0; i < 2; ++i)
        {
            fvm_value_undefined(&value);
            value.type = FVM_VALUE_OBJECT;
            value.object = proxy;
            /* Queue transport retains one native proxy shared by both workers. */
            assert(fvm_channel_send_value(client, input, &value, 0) == FVM_OK);
            memset(&tasks[i], 0, sizeof(tasks[i]));
            tasks[i].local = local;
            tasks[i].input = input;
            tasks[i].ready = ready;
            tasks[i].go = go;
            tasks[i].remote_first = i;
            fvm_value_undefined(&value);
            assert(fvm_worker_run(&cfg, run, &value, &tasks[i], &workers[i]) == FVM_OK);
        }
        for (i = 0; i < 2; ++i)
        {
            assert(fvm_channel_receive_owned(ready, &value, 1) == FVM_OK);
            fvm_channel_message_release(&value);
        }
        fvm_value_number(&value, 1);
        assert(fvm_channel_send(go, &value) == FVM_OK);
        assert(fvm_channel_send(go, &value) == FVM_OK);
        cycles = remote_cycles = 0;
        for (attempt = 0; attempt < 5000; ++attempt)
        {
            assert(fvm_debug_mutex_detail_snapshot(rows, 16, &count) == FVM_OK);
            cycles = remote_cycles = 0;
            for (j = 0; j < count; ++j)
                if (rows[j].state.deadlocked)
                {
                    FVM_DebugMutex *state = &rows[j].state;
                    ++cycles;
                    assert(state->waiter == tasks[rows[j].remote ? 0 : 1].context);
                    assert(state->owner == tasks[rows[j].remote ? 1 : 0].context);
                    assert(state->line == (unsigned long)(rows[j].remote ? 12 : 22));
                    assert(state->owner_line == (unsigned long)(rows[j].remote ? 21 : 11));
                    assert(!strcmp(state->source, rows[j].remote ? "mixed-local.as" : "mixed-remote.as"));
                    if (rows[j].remote)
                    {
                        ++remote_cycles;
                        assert(!strcmp(rows[j].address, uri));
                    }
                }
            if (cycles == 2 && remote_cycles == 1)
                break;
            fvm_port_thread_sleep_us(1000);
        }
        assert(cycles == 2 && remote_cycles == 1);
        for (i = 0; i < 2; ++i)
            assert(fvm_worker_stop(workers[i]) == FVM_OK);
        for (i = 0; i < 2; ++i)
        {
            FVM_Result result = fvm_worker_join(workers[i], 0);
            assert(result == FVM_OK || result == FVM_ERROR || result == FVM_EXCEPTION);
            fvm_worker_destroy(workers[i]);
        }
        assert(fvm_debug_mutex_detail_snapshot(rows, 16, &count) == FVM_OK);
        for (j = 0; j < count; ++j)
            assert(!rows[j].state.waiter && !rows[j].state.owner && !rows[j].state.deadlocked);
        assert(fvm_mutex_touch(authority, &held) == FVM_OK && !held);
        assert(fvm_mutex_touch(local, &held) == FVM_OK && !held);
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
    assert(fvm_debug_mutex_detail_snapshot(0, 0, &count) == FVM_OK && !count);
#endif
    return 0;
}
