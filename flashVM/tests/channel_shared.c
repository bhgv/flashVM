#include <stdio.h>
#include <stdlib.h>
#include "flashvm/channel.h"


int main(void)
{
    FVM_Channel *a = 0, *b;
    FVM_Value in, out;
    if (!(fvm_channel_create(&a) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_retain(a) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    b = a;                  /* second owner, e.g. a worker context */
    fvm_channel_destroy(a); /* original owner relinquishes its handle */
    fvm_value_number(&in, 42);
    if (!(fvm_channel_send(b, &in) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_receive(b, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(out.type == FVM_VALUE_NUMBER && out.number == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_channel_release(b);
    return 0;
}
