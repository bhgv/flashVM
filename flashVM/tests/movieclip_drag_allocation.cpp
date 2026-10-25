/* Script/C drag option allocation failures restore roots and preserve complete selection state. */
#define main allocation_fixture_main
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
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned native = 0; native < 2; ++native)
        {
            FVM_Context *c = 0;
            FVM_Object *clip = 0, *previous = 0;
            FVM_Value args[5], out;
            unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
            swf[3] = (unsigned char)version;
            if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK &&
                  fvm_standard_install(c) == FVM_OK && fvm_movieclip_create(c, "clip", 1, &clip) == FVM_OK &&
                  fvm_gc_pin(c, clip) == FVM_OK && fvm_movieclip_create(c, "previous", 1, &previous) == FVM_OK &&
                  fvm_gc_pin(c, previous) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_boolean(args, 1);
            fvm_value_number(args + 1, 0);
            fvm_value_number(args + 2, 0);
            fvm_value_number(args + 3, 10);
            fvm_value_number(args + 4, 10);
            bool success = false;
            unsigned failures = 0;
            for (long boundary = 0; boundary < 2000 && !success; ++boundary)
            {
                if (!(fvm_exec_drag_begin(c, previous, 0, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_GCRootFrame *roots = c->gc_frames;
                allowance = boundary;
                FVM_Result r = native ? fvm_object_call(c, clip, "startDrag", 5, args, &out)
                                      : fvm_exec_drag_values(c, clip, args, 4, args + 1);
                allowance = -1;
                if (!(c->gc_frames == roots && !c->drag_updating && !c->gc_native_depth && !c->script_depth))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (r == FVM_OK)
                {
                    if (!(c->drag_clip == clip))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    success = true;
                }
                else
                {
                    if (!(r == FVM_OUT_OF_MEMORY && (c->drag_clip == clip || c->drag_clip == previous)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    ++failures;
                }
                if (c->drag_clip == clip)
                {
                    if (!(c->drag_lock_center && c->drag_constrained && c->drag_bounds.x_max == 10 &&
                          c->drag_bounds.y_max == 10))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(!c->drag_lock_center && !c->drag_constrained))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
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
