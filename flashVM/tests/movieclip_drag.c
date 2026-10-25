/* C89 native drag methods share value options, versioned truth, partial limits and transport IDs. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/input.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/transfer.h"


static void position(FVM_Context *c, FVM_Object *clip, double x, double y)
{
    FVM_Affine m;
    if (!(fvm_movieclip_get_matrix(c, clip, &m) == FVM_OK && m.x0 == x && m.y0 == y))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    unsigned profile, native;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (profile = 0; profile < 6; ++profile)
    {
        FVM_Context *c, *other;
        FVM_Object *clip, *stopper, *copy;
        FVM_Value a[5], out, ctor, proto, value;
        FVM_Affine original = {1, 0, 0, 1, 5, 7};
        FVM_Channel *channel;
        int truth = !profile || profile + 3 >= 7;
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
        if (!(fvm_standard_install(c) == FVM_OK && fvm_movieclip_create(c, "clip", 1, &clip) == FVM_OK &&
              fvm_gc_pin(c, clip) == FVM_OK && fvm_movieclip_create(c, "stopper", 1, &stopper) == FVM_OK &&
              fvm_gc_pin(c, stopper) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (native = 0; native < 2; ++native)
        {
            if (!(fvm_movieclip_set_matrix(c, clip, &original) == FVM_OK && fvm_input_set_pointer(c, 10, 20) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_string(a, "yes");
            if (!((native ? fvm_object_call(c, clip, "startDrag", 1, a, &out)
                          : fvm_exec_drag_values(c, clip, a, 0, 0)) == FVM_OK &&
                  fvm_exec_drag_update(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            position(c, clip, truth ? 10 : 5, truth ? 20 : 7);
            if (!(fvm_exec_drag_end(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_movieclip_set_matrix(c, clip, &original) == FVM_OK && fvm_input_set_pointer(c, 20, 30) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_boolean(a, 1);
        fvm_value_number(a + 1, 10);
        if (!(fvm_object_call(c, clip, "startDrag", 2, a, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED &&
              fvm_exec_drag_update(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        position(c, clip, 10, 0);
        if (!(fvm_object_call(c, stopper, "stopDrag", 0, 0, &out) == FVM_OK &&
              fvm_exec_drag_target(c, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_set_matrix(c, clip, &original) == FVM_OK && fvm_input_set_pointer(c, 0, 0) == FVM_OK &&
              fvm_object_call(c, clip, "startDrag", 0, 0, &out) == FVM_OK && fvm_input_set_pointer(c, 4, 6) == FVM_OK &&
              fvm_exec_drag_update(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        position(c, clip, 9, 13);
        if (!(fvm_exec_drag_values(c, clip, 0, 1, 0) == FVM_INVALID_ARGUMENT &&
              fvm_exec_drag_values(c, clip, 0, 5, a) == FVM_INVALID_ARGUMENT &&
              fvm_exec_drag_target(c, &out) == FVM_OK && out.object == clip))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_get_global(c, "MovieClip", &ctor) == FVM_OK &&
              fvm_object_get(c, ctor.object, "prototype", &proto) == FVM_OK &&
              fvm_object_call(c, proto.object, "startDrag", 0, 0, &out) == FVM_OK &&
              fvm_object_call(c, proto.object, "stopDrag", 0, 0, &out) == FVM_OK &&
              fvm_exec_drag_target(c, &out) == FVM_OK && out.object == clip))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = clip;
        if (!(fvm_channel_create(&channel) == FVM_OK && fvm_channel_send_value(c, channel, &value, 0) == FVM_OK &&
              fvm_channel_receive_value(other, channel, &out, 0) == FVM_OK && out.type == FVM_VALUE_OBJECT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        copy = out.object;
        fvm_channel_destroy(channel);
        if (!(fvm_gc_pin(other, copy) == FVM_OK && fvm_exec_drag_target(other, &out) == FVM_OK &&
              out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_boolean(a, 1);
        if (!(fvm_input_set_pointer(other, 1, 2) == FVM_OK &&
              fvm_object_call(other, copy, "startDrag", 1, a, &out) == FVM_OK && fvm_exec_drag_update(other) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        position(other, copy, 1, 2);
        if (!(fvm_object_call(other, copy, "stopDrag", 0, 0, &out) == FVM_OK &&
              fvm_exec_drag_target(c, &out) == FVM_OK && out.object == clip && fvm_exec_drag_end(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK))
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
