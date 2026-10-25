/* Missing-operand insertion restores execution and root scopes after every allocation failure. */
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
    for (unsigned variant = 0; variant < 6; ++variant)
    {
        for (unsigned version = 4; version <= 8; ++version)
        {
            FVM_Context *c = 0;
            unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
            swf[3] = (unsigned char)version;
            if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            const unsigned char unary[] = {0x50, 0x17, 0}, empty_binary[] = {0x0b, 0x17, 0},
                                one_binary[] = {0x96, 5, 0, 7, 7, 0, 0, 0, 0x0b, 0x17, 0};
            const unsigned char empty_extract[] = {0x15, 0x17, 0},
                                one_extract[] = {0x96, 5, 0, 7, 7, 0, 0, 0, 0x15, 0x17, 0},
                                two_extract[] = {0x96, 10, 0, 7, 2, 0, 0, 0, 7, 3, 0, 0, 0, 0x15, 0x17, 0};
            const unsigned char *code = variant == 0   ? unary
                                        : variant == 1 ? empty_binary
                                        : variant == 2 ? one_binary
                                        : variant == 3 ? empty_extract
                                        : variant == 4 ? one_extract
                                                       : two_extract;
            size_t size = variant == 0   ? sizeof(unary)
                          : variant == 1 ? sizeof(empty_binary)
                          : variant == 2 ? sizeof(one_binary)
                          : variant == 3 ? sizeof(empty_extract)
                          : variant == 4 ? sizeof(one_extract)
                                         : sizeof(two_extract);
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
