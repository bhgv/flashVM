/* Allocation failures during converted SetVariable retain inputs and restore roots. */
#define main existing_allocation_main
#include <stdio.h>
#include <stdlib.h>
#include "execution_services_allocation.cpp"

#undef main
#include "flashvm/execution.h"
#include "../src/core/runtime.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
    if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value key, value, out;
    fvm_value_number(&key, 17);
    fvm_value_number(&value, 42);
    bool success = false;
    unsigned failures = 0;
    for (long boundary = 0; boundary < 2000 && !success; ++boundary)
    {
        FVM_GCRootFrame *roots = c->gc_frames;
        unsigned depth = c->string_depth;
        allowance = boundary;
        FVM_Result r = fvm_exec_variable_set(c, 0, 0, 0, &key, &value);
        allowance = -1;
        if (!(c->gc_frames == roots && c->string_depth == depth))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (r == FVM_OK)
        {
            if (!(fvm_get_global(c, "17", &out) == FVM_OK && out.number == 42))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            success = true;
        }
        else
        {
            if (!(r == FVM_OUT_OF_MEMORY))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            ++failures;
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(success && failures))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
