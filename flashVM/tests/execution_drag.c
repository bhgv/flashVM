/* C89 native dragging handles parent transforms, clamping, delivery, removal and GC roots. */
#include <stdio.h>
#include "flashvm/execution.h"
#include "flashvm/input_queue.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include <math.h>
#include <stdlib.h>

static FVM_Object *expected_clip;
static unsigned calls;


static void position(FVM_Context *c, FVM_Object *clip, double x, double y)
{
    FVM_Affine m;
    if (!(fvm_movieclip_get_matrix(c, clip, &m) == FVM_OK && fabs(m.x0 - x) < 1e-8 && fabs(m.y0 - y) < 1e-8))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void receive(FVM_Context *c, const FVM_InputEvent *e, FVM_InputPhase phase, FVM_MouseButton button, void *raw)
{
    (void)phase;
    (void)button;
    (void)raw;
    position(c, expected_clip, (e->x - 20) / 2, (e->y - 30) / 3);
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++calls;
}


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    unsigned profile;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (profile = 0; profile < 6; ++profile)
    {
        FVM_Context *c, *other;
        FVM_Object *root, *clip, *foreign, *floating;
        FVM_Value a[2], out, v;
        FVM_Affine root_matrix = {2, 0, 0, 3, 20, 30}, child_matrix = {0, 1, -1, 0, 5, 7},
                   singular_parent = {0, 0, 0, 0, 20, 30}, m;
        FVM_ExecutionDragBounds bounds = {10, 10, 0, 0};
        FVM_InputEvent event;
        unsigned processed;
        unsigned long before, after;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (profile)
        {
            swf[3] = (unsigned char)(profile + 3);
            if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_standard_install(c) == FVM_OK && fvm_movieclip_create(c, "root", 1, &root) == FVM_OK &&
              fvm_gc_pin(c, root) == FVM_OK && fvm_movieclip_set_matrix(c, root, &root_matrix) == FVM_OK &&
              fvm_object_create(other, &foreign) == FVM_OK))
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
        clip = out.object;
        if (!(fvm_movieclip_set_matrix(c, clip, &child_matrix) == FVM_OK &&
              fvm_input_set_pointer(c, 20, 30) == FVM_OK && fvm_exec_drag_begin(c, clip, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        position(c, clip, 5, 7);
        if (!(fvm_exec_drag_begin(c, foreign, 0, 0) == FVM_INVALID_ARGUMENT &&
              fvm_exec_drag_target(c, &out) == FVM_OK && out.object == clip))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_drag_target(other, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_input_set_pointer(c, 24, 36) == FVM_OK &&
              fvm_movieclip_set_matrix(c, root, &singular_parent) == FVM_OK &&
              fvm_exec_drag_update(c) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        position(c, clip, 5, 7);
        if (!(fvm_movieclip_set_matrix(c, root, &root_matrix) == FVM_OK && fvm_exec_drag_update(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        position(c, clip, 7, 9);
        if (!(fvm_movieclip_get_matrix(c, clip, &m) == FVM_OK && fabs(m.xx) < 1e-8 && fabs(m.yx - 1) < 1e-8 &&
              fabs(m.xy + 1) < 1e-8 && fabs(m.yy) < 1e-8))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_drag_begin(c, clip, 0, &bounds) == FVM_OK && fvm_input_set_pointer(c, 100, 100) == FVM_OK &&
              fvm_exec_drag_update(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        position(c, clip, 10, 10);
        if (!(fvm_input_set_pointer(c, 40, 42) == FVM_OK && fvm_exec_drag_update(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        position(c, clip, 10, 10);
        if (!(fvm_input_set_pointer(c, 28, 36) == FVM_OK && fvm_exec_drag_update(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        position(c, clip, 9, 9);
        if (!(fvm_exec_drag_end(c) == FVM_OK && fvm_exec_drag_target(c, &out) == FVM_OK &&
              out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_drag_begin(c, clip, 1, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        position(c, clip, 9, 9);
        if (!(fvm_input_set_pointer(c, 36, 48) == FVM_OK && fvm_exec_drag_update(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        position(c, clip, 8, 6);
        if (!(fvm_exec_drag_end(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_input_set_pointer(c, 20, 30) == FVM_OK && fvm_exec_drag_begin(c, clip, 1, 0) == FVM_OK &&
              fvm_exec_drag_end(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        position(c, clip, 0, 0);
        expected_clip = clip;
        calls = 0;
        if (!(fvm_exec_drag_begin(c, clip, 1, 0) == FVM_OK && fvm_input_set_callback(c, receive, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        event.gesture = FVM_GESTURE_TAP;
        event.fingers = 1;
        event.direction = 0;
        event.x = 26;
        event.y = 42;
        if (!(fvm_input_dispatch(c, &event, FVM_INPUT_END) == FVM_OK && calls == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        position(c, clip, 3, 4);
        event.x = 30;
        event.y = 45;
        if (!(fvm_input_enqueue(c, &event, FVM_INPUT_END) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        position(c, clip, 3, 4);
        if (!(fvm_input_pump(c, 0, &processed) == FVM_OK && processed == 1 && calls == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        position(c, clip, 5, 5);
        if (!(fvm_exec_drag_target(c, &out) == FVM_OK && out.object == clip &&
              fvm_input_set_callback(c, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        bounds.x_min = HUGE_VAL;
        bounds.x_max = -HUGE_VAL;
        bounds.y_min = bounds.y_max = strtod("NaN", 0);
        if (!(fvm_exec_drag_begin(c, clip, 1, &bounds) == FVM_OK && fvm_exec_drag_update(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        position(c, clip, 0, 0);
        if (!(fvm_exec_detach_clip(c, clip) == FVM_OK && fvm_exec_drag_update(c) == FVM_OK &&
              fvm_exec_drag_target(c, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_create(c, "floating", 1, &floating) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&v, 73);
        if (!(fvm_object_set(c, floating, "alive", &v) == FVM_OK && fvm_exec_drag_begin(c, floating, 0, 0) == FVM_OK &&
              fvm_gc_collect(c, 0) == FVM_OK && fvm_exec_drag_target(c, &out) == FVM_OK && out.object == floating &&
              fvm_object_get(c, floating, "alive", &out) == FVM_OK && out.number == 73))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_object_count(c, &before) == FVM_OK && fvm_exec_drag_end(c) == FVM_OK &&
              fvm_gc_collect(c, 0) == FVM_OK && fvm_gc_object_count(c, &after) == FVM_OK && after < before))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(other);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
