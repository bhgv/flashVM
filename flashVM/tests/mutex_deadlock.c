#include <stdio.h>
#include <stdlib.h>
#include "flashvm/mutex.h"
#include "flashvm/channel.h"
#include "flashvm/debug.h"
#include "flashvm/worker.h"
#include "../ports/thread_port.h"
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
    if (!(fvm_debug_set_location(c, t->source, t->line) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_mutex_set(c, t->first, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&v, 1);
    if (!(fvm_channel_send(t->ready, &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_receive_owned(t->go, &v, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_channel_message_release(&v);
    r = fvm_mutex_set(c, t->second, 1);
    if (r == FVM_OK)
    {
        if (!(fvm_mutex_reset(c, t->second) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
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
    if (!(fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_mutex_create(c, &a) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_mutex_create(c, &b) == FVM_OK))
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
        if (!(fvm_worker_run(&cfg, run, &v, &tasks[i], &workers[i]) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    for (i = 0; i < 2; ++i)
    {
        if (!(fvm_channel_receive_owned(ready, &v, 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_channel_message_release(&v);
    }
    fvm_value_number(&v, 1);
    if (!(fvm_channel_send(go, &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_send(go, &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    cycles = 0;
    for (i = 0; i < 5000 && cycles != 2; ++i)
    {
        if (!(fvm_debug_mutex_snapshot(rows, 16, &count) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        cycles = 0;
        for (j = 0; j < count; ++j)
        {
            if (rows[j].deadlocked)
            {
                ++cycles;
                if (!(rows[j].owner_line == 10 || rows[j].owner_line == 11))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(rows[j].owner_source[0]))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(rows[j].waiter && rows[j].owner && rows[j].waiter != rows[j].owner))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(rows[j].line == 10 || rows[j].line == 11))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(!strcmp(rows[j].source, "worker-a.as") || !strcmp(rows[j].source, "worker-b.as")))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        if (cycles != 2)
        {
            fvm_port_thread_sleep_us(1000);
        }
    }
    if (!(cycles == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_debug_mutex_snapshot(0, 0, &count) == FVM_NOT_FOUND && count == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_worker_stop(workers[0]) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_worker_stop(workers[1]) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < 2; ++i)
    {
        FVM_Result r;
        r = fvm_worker_join(workers[i], 0);
        if (!(r == FVM_OK || r == FVM_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_worker_destroy(workers[i]);
    }
    if (!(fvm_debug_mutex_snapshot(rows, 16, &count) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (j = 0; j < count; ++j)
    {
        if (!(!rows[j].owner && !rows[j].waiter && !rows[j].deadlocked))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_mutex_touch(a, &held) == FVM_OK && !held))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_mutex_release(a);
    fvm_mutex_release(b);
    fvm_channel_destroy(ready);
    fvm_channel_destroy(go);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    if (!(fvm_debug_mutex_snapshot(0, 0, &count) == FVM_OK && count == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
