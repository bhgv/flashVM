#include "flashvm/mutex.h"
#include "flashvm/channel.h"
#include "flashvm/debug.h"
#include "flashvm/worker.h"
#include "../ports/thread_port.h"
#include <assert.h>
#include <string.h>
typedef struct Task
{
    FVM_Mutex *first, *second;
    FVM_Channel *ready, *go;
    const char *source;
    unsigned long line;
} Task;
static FVM_Result run(FVM_Context *c, const FVM_Value *in, FVM_Value *out, void *userdata)
{
    Task *t;
    FVM_Value v;
    FVM_Result r;
    (void)in;
    t = (Task *)userdata;
    assert(fvm_debug_set_location(c, t->source, t->line) == FVM_OK);
    assert(fvm_mutex_set(c, t->first, 1) == FVM_OK);
    fvm_value_number(&v, 1);
    assert(fvm_channel_send(t->ready, &v) == FVM_OK);
    assert(fvm_channel_receive_owned(t->go, &v, 1) == FVM_OK);
    fvm_channel_message_release(&v);
    r = fvm_mutex_set(c, t->second, 1);
    if (r == FVM_OK)
        assert(fvm_mutex_reset(c, t->second) == FVM_OK);
    /* First mutex intentionally held: interpreter teardown must release it. */
    fvm_value_undefined(out);
    return r;
}
int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    FVM_Context *c;
    FVM_Worker *workers[2];
    FVM_Channel *ready, *go;
    FVM_Mutex *a, *b;
    Task tasks[2];
    FVM_DebugMutex rows[16];
    FVM_Value v;
    size_t count, j;
    int i, cycles, held;
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_mutex_create(c, &a) == FVM_OK);
    assert(fvm_mutex_create(c, &b) == FVM_OK);
    assert(fvm_channel_create(&ready) == FVM_OK);
    assert(fvm_channel_create(&go) == FVM_OK);
    tasks[0].first = a;
    tasks[0].second = b;
    tasks[1].first = b;
    tasks[1].second = a;
    fvm_value_undefined(&v);
    for (i = 0; i < 2; ++i)
    {
        tasks[i].ready = ready;
        tasks[i].go = go;
        tasks[i].source = i ? "worker-b.as" : "worker-a.as";
        tasks[i].line = (unsigned long)(10 + i);
        assert(fvm_worker_run(&cfg, run, &v, &tasks[i], &workers[i]) == FVM_OK);
    }
    for (i = 0; i < 2; ++i)
    {
        assert(fvm_channel_receive_owned(ready, &v, 1) == FVM_OK);
        fvm_channel_message_release(&v);
    }
    fvm_value_number(&v, 1);
    assert(fvm_channel_send(go, &v) == FVM_OK);
    assert(fvm_channel_send(go, &v) == FVM_OK);
    cycles = 0;
    for (i = 0; i < 5000 && cycles != 2; ++i)
    {
        assert(fvm_debug_mutex_snapshot(rows, 16, &count) == FVM_OK);
        cycles = 0;
        for (j = 0; j < count; ++j)
            if (rows[j].deadlocked)
            {
                ++cycles;
                assert(rows[j].owner_line == 10 || rows[j].owner_line == 11);
                assert(rows[j].owner_source[0]);
                assert(rows[j].waiter && rows[j].owner && rows[j].waiter != rows[j].owner);
                assert(rows[j].line == 10 || rows[j].line == 11);
                assert(!strcmp(rows[j].source, "worker-a.as") ||
                       !strcmp(rows[j].source, "worker-b.as"));
            }
        if (cycles != 2)
            fvm_port_thread_sleep_us(1000);
    }
    assert(cycles == 2);
    assert(fvm_debug_mutex_snapshot(0, 0, &count) == FVM_NOT_FOUND && count == 2);
    assert(fvm_worker_stop(workers[0]) == FVM_OK);
    assert(fvm_worker_stop(workers[1]) == FVM_OK);
    for (i = 0; i < 2; ++i)
    {
        FVM_Result r;
        r = fvm_worker_join(workers[i], 0);
        assert(r == FVM_OK || r == FVM_ERROR);
        fvm_worker_destroy(workers[i]);
    }
    assert(fvm_debug_mutex_snapshot(rows, 16, &count) == FVM_OK);
    for (j = 0; j < count; ++j)
        assert(!rows[j].owner && !rows[j].waiter && !rows[j].deadlocked);
    assert(fvm_mutex_touch(a, &held) == FVM_OK && !held);
    fvm_mutex_release(a);
    fvm_mutex_release(b);
    fvm_channel_destroy(ready);
    fvm_channel_destroy(go);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    assert(fvm_debug_mutex_snapshot(0, 0, &count) == FVM_OK && count == 0);
    return 0;
}
