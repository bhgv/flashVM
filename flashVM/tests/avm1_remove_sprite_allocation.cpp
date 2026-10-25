/* Full Action removal restores activation roots even when post-commit target checks fail. */
#define main allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "execution_services_allocation.cpp"

#undef main
#include "../src/core/runtime.hpp"
#include "flashvm/avm1.h"
#include "flashvm/execution.h"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const unsigned char code[] = {0x96, 8, 0, 0, 'v', 'i', 'c', 't', 'i', 'm', 0, 0x25, 0};
    for (unsigned version = 4; version <= 8; ++version)
    {
        bool success = false;
        unsigned failures = 0;
        for (long boundary = 0; boundary < 2000 && !success; ++boundary)
        {
            FVM_Context *c = 0;
            FVM_Object *root = 0;
            FVM_Value args[2], out;
            unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
            swf[3] = (unsigned char)version;
            if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK &&
                  fvm_standard_install(c) == FVM_OK && fvm_movieclip_create(c, "root", 1, &root) == FVM_OK &&
                  fvm_gc_pin(c, root) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_string(args, "victim");
            fvm_value_number(args + 1, 2);
            if (!(fvm_object_call(c, root, "createEmptyMovieClip", 2, args, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *clip = out.object;
            if (!(fvm_gc_pin(c, clip) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_GCRootFrame *roots = c->gc_frames;
            allowance = boundary;
            FVM_Result r = fvm_avm1_execute_receiver(c, code, sizeof(code), root);
            allowance = -1;
            if (!(c->gc_frames == roots && !c->avm1_execution_depth && !c->script_depth && !c->avm1_target_active))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_exec_clip_detached(c, clip, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            bool committed = out.boolean != 0;
            if (!(fvm_object_get(c, root, "victim", &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(committed ? out.type == FVM_VALUE_UNDEFINED : out.type == FVM_VALUE_OBJECT && out.object == clip))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (r == FVM_OK)
            {
                if (!(committed))
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
            fvm_context_destroy(c);
        }
        if (!(success && failures))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_runtime_destroy(rt);
    return 0;
}
