/* Clock callback itself allocates nothing; surrounding public invocation recovers from every OOM. */
#define main allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "execution_services_allocation.cpp"

#undef main


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Value fn, out;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_get_global(c, "getTimer", &fn) == FVM_OK && fvm_gc_pin(c, fn.object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_NativeFunction callback = fn.object->invocable.callback;
    FVM_GCRootFrame *roots = c->gc_frames;
    size_t objects = c->objects.size(), strings = c->strings.size();
    allowance = 0;
    for (unsigned i = 0; i < 128; ++i)
    {
        if (!(callback(c, 0, 0, 0, &out, 0) == FVM_OK && out.type == FVM_VALUE_NUMBER))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    allowance = -1;
    if (!(c->objects.size() == objects && c->strings.size() == strings && c->gc_frames == roots))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned route = 0; route < 2; ++route)
    {
        bool success = false;
        for (long boundary = 0; boundary < 256 && !success; ++boundary)
        {
            allowance = boundary;
            FVM_Result r =
                route ? fvm_function_invoke(c, fn.object, 0, 0, 0, &out) : fvm_call(c, "getTimer", 0, 0, &out);
            allowance = -1;
            if (r == FVM_OK)
            {
                if (!(out.type == FVM_VALUE_NUMBER))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                success = true;
            }
            else
            {
                if (!(r == FVM_OUT_OF_MEMORY && out.type == FVM_VALUE_UNDEFINED))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(c->gc_frames == roots && !c->gc_native_depth && !c->script_depth && !c->avm1_execution_depth))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(success))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_gc_unpin(c, fn.object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
