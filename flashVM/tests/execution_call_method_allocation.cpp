#define main existing_allocation_main
#include <stdio.h>
#include <stdlib.h>
#include "execution_services_allocation.cpp"

#undef main


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Value owner, name, arg, out;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(&owner, "abc");
    fvm_value_string(&name, "charAt");
    fvm_value_number(&arg, 1);
    bool success = false;
    unsigned failures = 0;
    for (long boundary = 0; boundary < 2000 && !success; ++boundary)
    {
        FVM_GCRootFrame *roots = c->gc_frames;
        fvm_value_number(&out, 99);
        allowance = boundary;
        FVM_Result r = fvm_exec_call_method(c, 0, &owner, &name, 0, 1, &arg, &out);
        allowance = -1;
        if (!(c->gc_frames == roots))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (r == FVM_OK)
        {
            if (!(out.type == FVM_VALUE_STRING && out.string_size == 1 && out.string[0] == 'b'))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            success = true;
        }
        else
        {
            if (!(r == FVM_OUT_OF_MEMORY && out.type == FVM_VALUE_NUMBER && out.number == 99))
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
    if (!(success && failures > 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
