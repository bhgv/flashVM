/* C89 local pointer projection shares named/indexed access and destination-context transport. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/input.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/transfer.h"
#include <math.h>


static void close_to(double actual, double expected)
{
    if (!(fabs(actual - expected) < 1e-8))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static FVM_Result replaced(FVM_Context *c, FVM_Object *o, unsigned n, const FVM_Value *a, FVM_Value *out, void *raw)
{
    (void)c;
    (void)o;
    (void)n;
    (void)a;
    (void)raw;
    fvm_value_number(out, 999);
    return FVM_OK;
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
        FVM_Object *root, *clip, *plain;
        FVM_Value a[2], x, y, out, target, index, ctor, proto;
        FVM_Channel *channel;
        FVM_Affine root_matrix = {2, 0, 0, 3, 20, 30}, child_matrix = {0, 1, -1, 0, 5, 7},
                   singular = {0, 0, 0, 0, 5, 7};
        unsigned lock;
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
              fvm_gc_pin(c, root) == FVM_OK && fvm_object_create(c, &plain) == FVM_OK &&
              fvm_movieclip_set_matrix(c, root, &root_matrix) == FVM_OK))
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
        if (!(fvm_movieclip_set_matrix(c, clip, &child_matrix) == FVM_OK && fvm_input_set_pointer(c, 18, 63) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (lock = 0; lock < 2; ++lock)
        {
            fvm_value_boolean(&out, lock);
            if (!(fvm_object_set(c, clip, "_lockroot", &out) == FVM_OK &&
                  fvm_exec_clip_pointer(c, clip, &x, &y) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            close_to(x.number, 4);
            close_to(y.number, 6);
            if (!(fvm_object_get(c, clip, "_xmouse", &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            close_to(out.number, 4);
            if (!(fvm_object_get(c, clip, "_ymouse", &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            close_to(out.number, 6);
        }
        fvm_value_undefined(&target);
        target.type = FVM_VALUE_OBJECT;
        target.object = clip;
        fvm_value_number(&index, 20);
        if (!(fvm_exec_display_property(c, 0, &target, &index, 0, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        close_to(out.number, 4);
        fvm_value_number(&index, 21);
        if (!(fvm_exec_display_property(c, 0, &target, &index, 0, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        close_to(out.number, 6);
        fvm_value_number(&out, 91);
        if (!(fvm_object_set(c, clip, "_xmouse", &out) == FVM_OK &&
              fvm_exec_display_property(c, 1, &target, &index, &out, &out) == FVM_OK &&
              out.type == FVM_VALUE_UNDEFINED && fvm_exec_clip_pointer(c, clip, &x, &y) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        close_to(x.number, 4);
        close_to(y.number, 6);
        if (!(fvm_get_global(c, "MovieClip", &ctor) == FVM_OK &&
              fvm_object_get(c, ctor.object, "prototype", &proto) == FVM_OK &&
              fvm_object_get(c, proto.object, "_xmouse", &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&x, 73);
        fvm_value_number(&y, 74);
        if (!(fvm_exec_clip_pointer(c, clip, &x, &x) == FVM_INVALID_ARGUMENT && x.number == 73 &&
              fvm_exec_clip_pointer(c, plain, &x, &y) == FVM_INVALID_ARGUMENT && x.number == 73 && y.number == 74 &&
              fvm_exec_clip_pointer(c, proto.object, &x, &y) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_set_matrix(c, clip, &singular) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (profile)
        {
            if (!(fvm_exec_clip_pointer(c, clip, &x, &y) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            close_to(x.number, 0.9);
            close_to(y.number, 3.15);
        }
        else
        {
            if (!(fvm_exec_clip_pointer(c, clip, &x, &y) == FVM_INVALID_ARGUMENT && x.number == 73 && y.number == 74))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_movieclip_set_matrix(c, clip, &child_matrix) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_channel_create(&channel) == FVM_OK && fvm_channel_send_value(c, channel, &target, 0) == FVM_OK &&
              fvm_channel_receive_value(other, channel, &out, 0) == FVM_OK && out.type == FVM_VALUE_OBJECT &&
              fvm_gc_pin(other, out.object) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_channel_destroy(channel);
        if (!(fvm_input_set_pointer(other, 20, 30) == FVM_OK &&
              fvm_object_get(other, out.object, "_xmouse", &x) == FVM_OK &&
              fvm_object_get(other, out.object, "_ymouse", &y) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        close_to(x.number, -7);
        close_to(y.number, 5);
        if (!(fvm_object_define_accessor_rooted(c, clip, "_xmouse", replaced, 0, 0, 0) == FVM_OK &&
              fvm_object_get(c, clip, "_xmouse", &out) == FVM_OK && out.number == 999 &&
              fvm_exec_clip_pointer(c, clip, &x, &y) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        close_to(x.number, 4);
        fvm_value_number(&index, 20);
        if (!(fvm_exec_display_property(c, 0, &target, &index, 0, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        close_to(out.number, 4);
        {
            FVM_Affine identity = {1, 0, 0, 1, 0, 0};
            if (!(fvm_movieclip_set_matrix(c, root, &identity) == FVM_OK &&
                  fvm_movieclip_set_matrix(c, clip, &identity) == FVM_OK &&
                  fvm_input_set_pointer(c, 18.5, -19.5) == FVM_OK && fvm_exec_clip_pointer(c, clip, &x, &y) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            close_to(x.number, profile ? 18 : 18.5);
            close_to(y.number, profile ? -20 : -19.5);
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
