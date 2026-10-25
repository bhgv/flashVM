#include <stdio.h>
#include <stdlib.h>
#include "flashvm/mutex.h"
#include "flashvm/worker.h"
#include "flashvm/debug.h"


typedef struct Counter
{
    FVM_Mutex *mutex;
    unsigned long value;
} Counter;


static FVM_Result count(FVM_Context *c, const FVM_Value *input, FVM_Value *out, void *userdata)
{
    Counter *counter;
    unsigned int i;
    (void)input;
    counter = (Counter *)userdata;
    for (i = 0; i < 2000; ++i)
    {
        if (!(fvm_mutex_set(c, counter->mutex, 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        ++counter->value;
        if (!(fvm_mutex_reset(c, counter->mutex) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_value_number(out, 2000);
    return FVM_OK;
}


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    FVM_Context *c;
    FVM_Worker *workers[4];
    FVM_Value input, result;
    Counter counter;
    int i;
    size_t n;
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
    if (!(fvm_mutex_create(c, &counter.mutex) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    counter.value = 0;
    fvm_value_undefined(&input);
    for (i = 0; i < 4; ++i)
    {
        if (!(fvm_worker_run(&cfg, count, &input, &counter, &workers[i]) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    for (i = 0; i < 4; ++i)
    {
        if (!(fvm_worker_join(workers[i], &result) == FVM_OK && result.number == 2000))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_worker_destroy(workers[i]);
    }
    if (!(counter.value == 8000))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_mutex_release(counter.mutex);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    if (!(fvm_debug_mutex_snapshot(0, 0, &n) == FVM_OK && !n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
