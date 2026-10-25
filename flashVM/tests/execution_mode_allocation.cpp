/* Automatic compilation/cache insertion and native invocation failures restore roots and ownership. */
#define main allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "execution_services_allocation.cpp"

#undef main
#include "flashvm/execution_mode.h"


int main()
{
    if (!fvm_jit_available())
    {
        return 0;
    }
    const unsigned char body[] = {0x96, 5, 0, 7, 20, 0, 0, 0, 0x96, 5, 0, 7, 22, 0, 0, 0, 0x0a, 0x3e, 0};
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Object *fn = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK &&
          fvm_function_create_avm1(c, body, sizeof(body), 0, 0, 0, &fn) == FVM_OK && fvm_gc_pin(c, fn) == FVM_OK &&
          fvm_execution_set_mode(c, FVM_EXECUTION_JIT, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bool success = false;
    unsigned failures = 0;
    for (long boundary = 0; boundary < 3000 && !success; ++boundary)
    {
        FVM_Value out;
        FVM_GCRootFrame *roots = c->gc_frames;
        allowance = boundary;
        FVM_Result r = fvm_function_invoke(c, fn, 0, 0, 0, &out);
        allowance = -1;
        if (!(c->gc_frames == roots && !c->gc_native_depth && !c->script_depth && !c->avm1_execution_depth))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (r == FVM_OK)
        {
            if (!(out.number == 42))
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
