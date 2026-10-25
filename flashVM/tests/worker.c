#include <stdio.h>
#include <stdlib.h>
#include "flashvm/worker.h"
#include <string.h>


static FVM_Result task(FVM_Context *ctx, const FVM_Value *in, FVM_Value *out, void *ud)
{
    FVM_Value value;
    (void)ud;
    if (!ctx || in->type != FVM_VALUE_STRING || strcmp(in->string, "hello"))
    {
        return FVM_ERROR;
    }
    fvm_value_number(&value, 42);
    if (fvm_set_global(ctx, "answer", &value) != FVM_OK)
    {
        return FVM_ERROR;
    }
    return fvm_get_global(ctx, "answer", out);
}


int main(void)
{
    FVM_Config config;
    FVM_Value in, out;
    FVM_Worker *worker = 0;
    if (!(fvm_config_init(&config) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_string(&in, "hello") == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_worker_run(&config, task, &in, 0, &worker) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_worker_join(worker, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(out.type == FVM_VALUE_NUMBER && out.number == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_worker_destroy(worker);
    return 0;
}
