/* Drag transform publication and effective mouse deltas survive every allocation failure. */
#define main allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "execution_services_allocation.cpp"

#undef main
#include "flashvm/execution.h"
#include "flashvm/input.h"
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
        for (unsigned mode = 0; mode < 3; ++mode)
        {
            FVM_Context *c = 0;
            FVM_Object *root = 0;
            FVM_Value a[2], out;
            FVM_Affine parent = {2, 0, 0, 3, 20, 30}, original = {1, 0, 0, 1, 5, 7}, current;
            FVM_InputEvent event;
            unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
            swf[3] = (unsigned char)version;
            if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK &&
                  fvm_standard_install(c) == FVM_OK && fvm_movieclip_create(c, "root", 1, &root) == FVM_OK &&
                  fvm_gc_pin(c, root) == FVM_OK && fvm_movieclip_set_matrix(c, root, &parent) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_string(a, "clip");
            fvm_value_number(a + 1, 1);
            if (!(fvm_object_call(c, root, "createEmptyMovieClip", 2, a, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *clip = out.object;
            event.gesture = FVM_GESTURE_TAP;
            event.fingers = 1;
            event.direction = 0;
            event.x = 24;
            event.y = 36;
            bool success = false;
            unsigned failures = 0;
            for (long boundary = 0; boundary < 2000 && !success; ++boundary)
            {
                if (!(fvm_movieclip_set_matrix(c, clip, &original) == FVM_OK &&
                      fvm_input_set_pointer(c, 20, 30) == FVM_OK && fvm_exec_drag_begin(c, clip, 0, 0) == FVM_OK &&
                      fvm_input_set_pointer(c, 24, 36) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_GCRootFrame *roots = c->gc_frames;
                allowance = boundary;
                FVM_Result r = mode == 0   ? fvm_exec_drag_update(c)
                               : mode == 1 ? fvm_input_dispatch(c, &event, FVM_INPUT_UPDATE)
                                           : fvm_exec_drag_end(c);
                allowance = -1;
                if (!(c->gc_frames == roots && !c->drag_updating && !c->gc_native_depth && !c->script_depth &&
                      fvm_movieclip_get_matrix(c, clip, &current) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (r == FVM_OK)
                {
                    if (!(current.x0 == 7 && current.y0 == 9 && (mode == 2 ? !c->drag_clip : c->drag_clip == clip)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    success = true;
                }
                else
                {
                    if (!(r == FVM_OUT_OF_MEMORY && current.x0 == 5 && current.y0 == 7 && c->drag_clip == clip &&
                          c->drag_last_world_x == 20 && c->drag_last_world_y == 30))
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
