/* Missing fixed call operands restores execution/root scopes at every allocation failure. */
#define main allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "execution_services_allocation.cpp"

#undef main
#include "../src/core/runtime.hpp"
#include "flashvm/avm1.h"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned variant = 0; variant < 10; ++variant)
        {
            FVM_Context *c = 0;
            unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
            swf[3] = (unsigned char)version;
            if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            const unsigned char ops[] = {0x3d, 0x40, 0x52, 0x53};
            unsigned op = variant < 4 ? variant : variant < 8 ? variant - 4 : variant - 6;
            unsigned char code[16];
            size_t size = 0;
            if (variant >= 8)
            {
                code[size++] = 0x96;
                code[size++] = 1;
                code[size++] = 0;
                code[size++] = 3;
            }
            if (variant >= 4)
            {
                code[size++] = 0x96;
                code[size++] = 5;
                code[size++] = 0;
                code[size++] = 7;
                code[size++] = 7;
                code[size++] = 0;
                code[size++] = 0;
                code[size++] = 0;
            }
            code[size++] = ops[op];
            code[size++] = 0;
            bool success = false;
            unsigned failures = 0;
            for (long boundary = 0; boundary < 2000 && !success; ++boundary)
            {
                FVM_GCRootFrame *roots = c->gc_frames;
                allowance = boundary;
                FVM_Result r = fvm_avm1_execute(c, code, size);
                allowance = -1;
                if (!(c->gc_frames == roots && !c->avm1_execution_depth && !c->script_depth))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (r == FVM_OK)
                {
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
        }
    }
    fvm_runtime_destroy(rt);
    return 0;
}
