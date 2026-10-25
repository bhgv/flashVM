#include <stdio.h>
#include "flashvm/debug.h"
#include "../src/core/channel_packet.h"
#include <stdlib.h>
#include <string.h>


static FVM_Result request(void *raw,
                          FVM_Context *c,
                          int op,
                          const unsigned char *input,
                          size_t size,
                          unsigned char **output,
                          size_t *length,
                          int blocking)
{
    int mode = *(int *)raw;
    (void)c;
    (void)blocking;
    if (output)
    {
        *output = 0;
    }
    if (length)
    {
        *length = 0;
    }
    if (op == 7)
    {
        if (!(size == 1 && input[0] <= 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (mode == 1)
        {
            *output = (unsigned char *)calloc(1, 1);
            if (!(*output))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            *length = 1;
        }
        return FVM_OK;
    }
    if (op == 9)
    {
        if (!(size == 4 && input[0] == 0 && input[1] == 0 && input[2] == 0 && input[3] == 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (mode == 4)
        {
            return FVM_OUT_OF_MEMORY;
        }
        if (mode == 5)
        {
            return FVM_NOT_FOUND;
        }
        *length = mode == 1 ? 1 : 16;
        if (mode == 2)
        {
            return FVM_OK;
        }
        *output = (unsigned char *)calloc(*length, 1);
        if (!(*output))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        return FVM_OK;
    }
    if (op != 8)
    {
        return FVM_OK;
    }
    if (mode == 4)
    {
        return FVM_OUT_OF_MEMORY;
    }
    if (mode == 5)
    {
        return FVM_NOT_FOUND;
    }
    *length = mode == 1 ? 11 : 12;
    if (mode == 2)
    {
        return FVM_OK;
    }
    *output = (unsigned char *)calloc(12, 1);
    if (!(*output))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    (*output)[3] = 4;
    (*output)[7] = 5;
    (*output)[11] = (unsigned char)(mode == 3 ? 4 : 3);
    return FVM_OK;
}


static void release(void *raw)
{
    (void)raw;
}


int main(void)
{
    int mode = 0;
    FVM_Channel *ch;
    FVM_DebugChannel state;
    FVM_Config cfg;
    FVM_Runtime *rt;
    FVM_Context *c;
    FVM_Value value;
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
    if (!(fvm_channel_create_remote(&mode, request, release, "adapter:test", &ch) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_debug_channel_state(ch, &state) == FVM_OK && state.pending == 4 && state.capacity == 5 && state.closed &&
          state.frozen && state.identity == fvm_channel_identity(ch)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (mode = 1; mode <= 5; ++mode)
    {
        FVM_Result expected = mode == 4 ? FVM_OUT_OF_MEMORY : mode == 5 ? FVM_NOT_FOUND : FVM_ERROR;
        state.pending = 987;
        state.identity = 555;
        if (!(fvm_debug_channel_state(ch, &state) == expected && state.pending == 987 && state.identity == 555))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    mode = 1;
    if (!(fvm_debug_channel_freeze(ch, 1) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    mode = 0;
    if (!(fvm_debug_channel_freeze(ch, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_debug_channel_freeze(ch, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (mode = 1; mode <= 5; ++mode)
    {
        FVM_Result expected = mode == 4 ? FVM_OUT_OF_MEMORY : mode == 5 ? FVM_NOT_FOUND : FVM_INVALID_ARGUMENT;
        if (!(fvm_debug_channel_message(c, ch, 3, &value) == expected))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(value.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    fvm_channel_release(ch);
    return 0;
}
