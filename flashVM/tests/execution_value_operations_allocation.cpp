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
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value a, b, out;
    fvm_value_number(&a, 1);
    fvm_value_number(&b, 2);
    for (unsigned operation = 0; operation < 3; ++operation)
    {
        bool success = false;
        unsigned failures = 0;
        for (long boundary = 0; boundary < 100 && !success; ++boundary)
        {
            FVM_GCRootFrame *roots = c->gc_frames;
            fvm_value_number(&out, 99);
            allowance = boundary;
            FVM_Result r = operation == 0   ? fvm_exec_logical(c, FVM_EXEC_LOGICAL_AND, &a, &b, &out)
                           : operation == 1 ? fvm_exec_legacy_compare(c, FVM_EXEC_LEGACY_LESS, &a, &b, &out)
                                            : fvm_exec_delete_member(c, &a, &b, &out);
            allowance = -1;
            if (!(c->gc_frames == roots))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (r == FVM_OK)
            {
                if (!(out.type == FVM_VALUE_BOOLEAN))
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
        }
        if (!(success && failures > 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    unsigned char swf[] = {'F', 'W', 'S', 8, 18, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0x40, 0, 0, 0};
    if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned count = 99;
    FVM_GCRootFrame *roots = c->gc_frames;
    allowance = 0;
    if (!(fvm_exec_argument_count(c, &a, 1, &count) == FVM_OUT_OF_MEMORY))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = -1;
    if (!(count == 99 && c->gc_frames == roots))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_exec_argument_count(c, &a, 1, &count) == FVM_OK && count == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&out, 99);
    allowance = 0;
    if (!(fvm_exec_to_string(c, &a, &out) == FVM_OUT_OF_MEMORY))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = -1;
    if (!(out.type == FVM_VALUE_NUMBER && out.number == 99 && c->gc_frames == roots))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_exec_to_string(c, &a, &out) == FVM_OK && out.type == FVM_VALUE_STRING))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
