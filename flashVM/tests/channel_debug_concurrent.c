#include "flashvm/extensions.h"
#include "flashvm/mutex.h"
#include "flashvm/transfer.h"
#include "flashvm/worker.h"
#include "flashvm/debug.h"
#include "../ports/thread_port.h"
#include <assert.h>
#include <string.h>
typedef struct Task
{
    FVM_Channel *channel;
    FVM_Mutex *mutex;
    unsigned long received;
} Task;
static FVM_Result send(FVM_Context *c, Task *task, unsigned long first, unsigned long end)
{
    FVM_Object *graph, *wrapper;
    FVM_Value value, number;
    unsigned long i;
    assert(fvm_object_create(c, &graph) == FVM_OK);
    assert(fvm_extensions_wrap_mutex(c, task->mutex, &wrapper) == FVM_OK);
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = wrapper;
    assert(fvm_object_set(c, graph, "lock", &value) == FVM_OK);
    value.object = graph;
    for (i = first; i < end; ++i)
    {
        fvm_value_number(&number, (double)i);
        assert(fvm_object_set(c, graph, "sequence", &number) == FVM_OK);
        assert(fvm_channel_send_value(c, task->channel, &value, 1) == FVM_OK);
    }
    return FVM_OK;
}
static FVM_Result producer(FVM_Context *c, const FVM_Value *in, FVM_Value *out, void *userdata)
{
    (void)in;
    fvm_value_undefined(out);
    return send(c, (Task *)userdata, 1, 400);
}
static FVM_Result consumer(FVM_Context *c, const FVM_Value *in, FVM_Value *out, void *userdata)
{
    Task *task;
    FVM_Value graph, value;
    FVM_Object *canonical, *last;
    unsigned long i;
    (void)in;
    task = (Task *)userdata;
    canonical = 0;
    for (i = 0; i < 400; ++i)
    {
        assert(fvm_channel_receive_value(c, task->channel, &graph, 1) == FVM_OK);
        last = graph.object;
        assert(fvm_object_get(c, last, "sequence", &value) == FVM_OK && value.number == i);
        assert(fvm_object_get(c, last, "lock", &value) == FVM_OK);
        if (!canonical)
            canonical = value.object;
        assert(value.object == canonical);
        assert(fvm_mutex_set(c, task->mutex, 1) == FVM_OK);
        ++task->received;
        assert(fvm_mutex_reset(c, task->mutex) == FVM_OK);
    }
    fvm_value_number(out, 400);
    return FVM_OK;
}
int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    FVM_Context *c;
    FVM_Worker *p, *q;
    FVM_WorkerState ps, qs;
    FVM_Value input, snapshot, value;
    FVM_DebugChannel state;
    Task task;
    unsigned int i;
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_channel_create(&task.channel) == FVM_OK);
    assert(fvm_channel_set_capacity(task.channel, 16) == FVM_OK);
    assert(fvm_mutex_create(c, &task.mutex) == FVM_OK);
    /* Primitive C receives must respect freezing; inspection owns its string. */
    assert(fvm_value_string(&input, "frozen string") == FVM_OK);
    assert(fvm_channel_send(task.channel, &input) == FVM_OK);
    assert(fvm_debug_channel_freeze(task.channel, 1) == FVM_OK);
    assert(fvm_channel_try_receive(task.channel, &value) == FVM_NOT_FOUND);
    assert(fvm_channel_receive_owned(task.channel, &value, 0) == FVM_NOT_FOUND);
    assert(fvm_debug_channel_message(c, task.channel, 0, &snapshot) == FVM_OK);
    assert(snapshot.type == FVM_VALUE_STRING && !strcmp(snapshot.string, "frozen string"));
    assert(fvm_debug_channel_freeze(task.channel, 0) == FVM_OK);
    assert(fvm_channel_receive_owned(task.channel, &value, 0) == FVM_OK);
    fvm_channel_message_release(&value);
    assert(!strcmp(snapshot.string, "frozen string"));
    task.received = 0;
    assert(send(c, &task, 0, 1) == FVM_OK);
    assert(fvm_debug_channel_freeze(task.channel, 1) == FVM_OK);
    assert(fvm_debug_channel_message(c, task.channel, 0, &snapshot) == FVM_OK);
    assert(fvm_debug_channel_state(task.channel, &state) == FVM_OK && state.pending == 1);
    fvm_value_undefined(&input);
    assert(fvm_worker_run(&cfg, producer, &input, &task, &p) == FVM_OK);
    assert(fvm_worker_run(&cfg, consumer, &input, &task, &q) == FVM_OK);
    ps = qs = FVM_WORKER_RUNNING;
    for (i = 0; i < 5000 && (ps != FVM_WORKER_FINISHED || qs != FVM_WORKER_FINISHED); ++i)
    {
        FVM_Result r;
        assert(fvm_debug_channel_freeze(task.channel, 1) == FVM_OK);
        r = fvm_debug_channel_message(c, task.channel, 0, &snapshot);
        assert(r == FVM_OK || r == FVM_NOT_FOUND);
        if (r == FVM_OK)
        {
            assert(fvm_object_get(c, snapshot.object, "sequence", &value) == FVM_OK);
            assert(value.number >= 0 && value.number < 400);
        }
        assert(fvm_debug_channel_freeze(task.channel, 0) == FVM_OK);
        assert(fvm_worker_state(p, &ps) == FVM_OK);
        assert(fvm_worker_state(q, &qs) == FVM_OK);
        fvm_port_thread_sleep_us(1000);
    }
    assert(ps == FVM_WORKER_FINISHED && qs == FVM_WORKER_FINISHED);
    assert(fvm_worker_join(p, 0) == FVM_OK);
    assert(fvm_worker_join(q, &value) == FVM_OK && value.number == 400);
    assert(task.received == 400);
    assert(fvm_debug_channel_state(task.channel, &state) == FVM_OK && !state.pending);
    /* Closing preserves a frozen packet; explicit resume permits draining it. */
    fvm_value_number(&input, 7);
    assert(fvm_channel_send(task.channel, &input) == FVM_OK);
    assert(fvm_debug_channel_freeze(task.channel, 1) == FVM_OK);
    fvm_channel_close(task.channel);
    assert(fvm_channel_try_receive(task.channel, &value) == FVM_ERROR);
    assert(fvm_debug_channel_message(c, task.channel, 0, &snapshot) == FVM_OK &&
           snapshot.number == 7);
    assert(fvm_debug_channel_freeze(task.channel, 0) == FVM_OK);
    assert(fvm_channel_receive_owned(task.channel, &value, 0) == FVM_OK && value.number == 7);
    fvm_channel_message_release(&value);
    fvm_worker_destroy(p);
    fvm_worker_destroy(q);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    fvm_channel_destroy(task.channel);
    fvm_mutex_release(task.mutex);
    return 0;
}
