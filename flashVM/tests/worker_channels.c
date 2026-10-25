#include <stdio.h>
#include <stdlib.h>
#include "flashvm/worker.h"
#include "flashvm/channel.h"
#include <string.h>


typedef struct Exchange
{
    FVM_Channel *requests;
    FVM_Channel *replies;
} Exchange;


/* The callback runs in its own VM. The channels, not Flash objects,
 * are the only shared data between this context and the host. */
static FVM_Result exchange_worker(FVM_Context *ctx, const FVM_Value *input, FVM_Value *output, void *userdata)
{
    Exchange *ex = (Exchange *)userdata;
    FVM_Value request, reply;
    FVM_Result r;
    (void)ctx;
    (void)input;
    r = fvm_channel_receive(ex->requests, &request);
    if (r != FVM_OK)
    {
        return r;
    }
    if (request.type != FVM_VALUE_STRING)
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_number(&reply, (double)strlen(request.string));
    r = fvm_channel_send(ex->replies, &reply);
    if (r == FVM_OK)
    {
        fvm_value_number(output, 42.0);
    }
    return r;
}


int main(void)
{
    FVM_Config cfg;
    FVM_Worker *worker = 0;
    FVM_Value input, message, reply, output;
    Exchange ex;
    int available;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_create(&ex.requests) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_create(&ex.replies) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_try_receive(ex.replies, &reply) == FVM_NOT_FOUND))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&input);
    if (!(fvm_worker_run_suspended(&cfg, exchange_worker, &input, &ex, &worker) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_string(&message, "hello") == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_send(ex.requests, &message) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_worker_start(worker) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_receive(ex.replies, &reply) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(reply.type == FVM_VALUE_NUMBER && reply.number == 5.0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_worker_join(worker, &output) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(output.type == FVM_VALUE_NUMBER && output.number == 42.0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_worker_destroy(worker);
    fvm_channel_close(ex.requests);
    if (!(fvm_channel_try_receive(ex.requests, &reply) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_available(ex.requests, &available) == FVM_OK && !available))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_channel_destroy(ex.requests);
    fvm_channel_destroy(ex.replies);
    return 0;
}
