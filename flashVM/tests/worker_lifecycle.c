#include <stdio.h>
#include <stdlib.h>
#include "flashvm/worker.h"


static FVM_Result callback(FVM_Context *ctx, const FVM_Value *in, FVM_Value *out, void *userdata)
{
    (void)userdata;
    if (!ctx || in->type != FVM_VALUE_NUMBER)
    {
        return FVM_ERROR;
    }
    fvm_value_number(out, in->number + 1);
    return FVM_OK;
}


int main(void)
{
    FVM_Config cfg;
    FVM_Worker *w = 0;
    FVM_WorkerState state;
    FVM_Value in, out;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&in, 41);
    if (!(fvm_worker_run_suspended(&cfg, callback, &in, 0, &w) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_worker_state(w, &state) == FVM_OK && state == FVM_WORKER_SUSPENDED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_worker_start(w) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_worker_join(w, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(out.type == FVM_VALUE_NUMBER && out.number == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_worker_state(w, &state) == FVM_OK && state == FVM_WORKER_JOINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_worker_destroy(w);
    return 0;
}
