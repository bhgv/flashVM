#include <stdio.h>
#include <stdlib.h>
#include "flashvm/worker.h"
#include "flashvm/channel.h"


typedef struct Data
{
    FVM_Channel *channel, *entered;
    int send;
} Data;


static FVM_Result blocked(FVM_Context *c, const FVM_Value *in, FVM_Value *out, void *ud)
{
    Data *d = (Data *)ud;
    FVM_Value v;
    FVM_Result r;
    (void)c;
    (void)in;
    fvm_value_number(&v, 1);
    if (!(fvm_channel_send(d->entered, &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    r = d->send ? fvm_channel_send(d->channel, &v) : fvm_channel_receive_owned(d->channel, &v, 1);
    fvm_value_number(out, (double)r);
    return FVM_OK;
}


int main(void)
{
    FVM_Config cfg;
    FVM_Worker *w;
    FVM_Value v, out;
    Data d;
    unsigned int i;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&v, 1);
    for (i = 0; i < 40; ++i)
    {
        d.send = (int)(i % 2);
        if (!(fvm_channel_create(&d.channel) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_channel_create(&d.entered) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_channel_set_capacity(d.channel, 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (d.send)
        {
            if (!(fvm_channel_send(d.channel, &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_worker_run(&cfg, blocked, &v, &d, &w) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_channel_receive(d.entered, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_channel_close(d.channel);
        if (!(fvm_worker_join(w, &out) == FVM_OK && out.number == FVM_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_worker_destroy(w);
        fvm_channel_release(d.channel);
        fvm_channel_release(d.entered);
    }
    return 0;
}
