/* Every preparation/publication failure preserves the matching destination matrix and color. */
#define main allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "execution_services_allocation.cpp"

#undef main
#define FVM_TRANSFORM_FIXTURE_ONLY
#include "standard_transform.cpp"
#include "flashvm/execution.h"
#include "../src/core/runtime.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned native = 0; native < 2; ++native)
    {
        FVM_Context *c = 0;
        FVM_Object *source = 0, *dest = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK &&
              fvm_movieclip_create(c, "source", 1, &source) == FVM_OK && fvm_gc_pin(c, source) == FVM_OK &&
              fvm_movieclip_create(c, "dest", 1, &dest) == FVM_OK && fvm_gc_pin(c, dest) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        double before[] = {1, 0, 0, 1, 0, 0}, after[] = {2, 0.25, 0.5, 3, 4, 5}, oldcolor[] = {1, 1, 1, 1, 0, 0, 0, 0},
               newcolor[] = {0.5, 1, 2, 1.5, 11, 12, 13, -40};
        FVM_Object *source_t = property(c, source, "transform"), *dest_t = property(c, dest, "transform");
        assign(c, source_t, "matrix", construct(c, "flash.geom.Matrix", 6, after));
        assign(c, source_t, "colorTransform", construct(c, "flash.geom.ColorTransform", 8, newcolor));
        FVM_Value arg;
        ref(&arg, source_t);
        bool success = false;
        unsigned failures = 0;
        FVM_GCRootFrame *roots = c->gc_frames;
        for (long failure = 0; failure < 4096 && !success; ++failure)
        {
            allowance = failure;
            FVM_Result r =
                native ? fvm_object_set(c, dest, "transform", &arg) : fvm_exec_clip_copy_transform(c, dest, source);
            allowance = -1;
            if (!(r == FVM_OK || r == FVM_OUT_OF_MEMORY))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            success = r == FVM_OK;
            if (!success)
            {
                ++failures;
            }
            if (!(c->gc_frames == roots && !c->gc_native_depth && !c->script_depth))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            matrix(c, property(c, dest_t, "matrix"), success ? after : before);
            color(c, property(c, dest_t, "colorTransform"), success ? newcolor : oldcolor);
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
    fvm_runtime_destroy(rt);
    return 0;
}
