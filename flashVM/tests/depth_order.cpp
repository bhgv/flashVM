/* Shared heap-sort oracle: odd/even workspaces, duplicates, zero work and exact exhaustion boundary. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/core/depth_order.hpp"


/* Check small rotated input sequences without assuming hash ordering or a library sorting algorithm. */
int main()
{
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK &&
          fvm_context_create(runtime, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned size = 0; size <= 31; ++size)
    {
        for (unsigned rotate = 0; rotate < (size ? size : 1); ++rotate)
        {
            fvm_detail::vector<unsigned long> values;
            for (unsigned i = 0; i < size; ++i)
            {
                values.push_back((size - i + rotate) % size);
            }
            unsigned long units = 0;
            {


                FVM_ExecutionWorkScope work(c);
                if (!(fvm_depth_order::sort(values, work) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                units = c->avm1_steps;
            }
            for (unsigned i = 0; i < size; ++i)
            {
                if (!(values[i] == i))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (size < 2)
            {
                if (!(!units))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            for (unsigned long limit = 1; limit <= units; ++limit)
            {
                values.clear();
                for (unsigned i = 0; i < size; ++i)
                {
                    values.push_back((size - i + rotate) % size);
                }
                runtime->config.instruction_limit = limit;
                {


                    FVM_ExecutionWorkScope work(c);
                    if (!(fvm_depth_order::sort(values, work) == (limit == units ? FVM_OK : FVM_ERROR)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(c->avm1_steps == limit))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }
            runtime->config.instruction_limit = 0;
        }
    }
    fvm_detail::vector<unsigned long> repeated;
    for (unsigned i = 0; i < 32; ++i)
    {
        repeated.push_back(i % 3);
    }
    {


        FVM_ExecutionWorkScope work(c);
        if (!(fvm_depth_order::sort(repeated, work) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    for (size_t i = 1; i < repeated.size(); ++i)
    {
        if (!(repeated[i - 1] <= repeated[i]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    /* A full raster-sized reverse list stays below an explicit n log n work bound. */
    repeated.clear();
    for (unsigned long i = 65536; i; --i)
    {
        repeated.push_back(i - 1);
    }
    {


        FVM_ExecutionWorkScope work(c);
        if (!(fvm_depth_order::sort(repeated, work) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(c->avm1_steps < 65536UL * 40UL))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    for (size_t i = 0; i < repeated.size(); ++i)
    {
        if (!(repeated[i] == i))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    /* Disabled accounting must preserve an enclosing caller's exhausted budget. */
    c->avm1_steps = 71;
    runtime->config.instruction_limit = 1;
    {


        FVM_ExecutionWorkScope work(c, false);
        if (!(fvm_depth_order::sort(repeated, work) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(c->avm1_steps == 71))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(!c->avm1_execution_depth))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(runtime);
}
