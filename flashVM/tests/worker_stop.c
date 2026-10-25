#include <stdio.h>
#include <stdlib.h>
#include "flashvm/worker.h"


typedef struct WorkerData
{
    FVM_Worker *handle;
} WorkerData;


static FVM_Result callback(FVM_Context *ctx, const FVM_Value *input, FVM_Value *result, void *userdata)
{
    WorkerData *data = (WorkerData *)userdata;
    int stop = 0;
    (void)ctx;
    if (!(fvm_worker_stop_requested(data->handle, &stop) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(result, input->number + (stop ? 1 : 0));
    return FVM_OK;
}


int main(void)
{
    FVM_Config cfg;
    FVM_Worker *workers[2];
    WorkerData data[2];
    FVM_Value input, outputs[2];
    unsigned int i;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&input, 41);
    for (i = 0; i < 2; ++i)
    {
        data[i].handle = 0;
        if (!(fvm_worker_run_suspended(&cfg, callback, &input, &data[i], &workers[i]) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        data[i].handle = workers[i];
        if (!(fvm_worker_stop(workers[i]) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_worker_join_all(workers, 2, outputs) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < 2; ++i)
    {
        if (!(outputs[i].type == FVM_VALUE_NUMBER && outputs[i].number == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_worker_destroy(workers[i]);
    }
    return 0;
}
