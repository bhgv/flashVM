#define main movieclip_fixture_main
#include <stdlib.h>
#include "standard_movieclip.c"

#undef main
#include <stdio.h>


static void hit(FVM_Context *c, FVM_Object *o, double x, double y, int shape, int expected)
{
    FVM_Value args[3], out;
    fvm_value_number(args, x);
    fvm_value_number(args + 1, y);
    fvm_value_boolean(args + 2, shape);
    if (!(fvm_object_call(c, o, "hitTest", 3, args, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(out.type == FVM_VALUE_BOOLEAN && out.boolean == expected))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void verify(FVM_Context *c, FVM_Object *o)
{
    hit(c, o, 27, 22, 1, 1);
    hit(c, o, 33, 28, 0, 1);
    hit(c, o, 33, 28, 1, 0);
    hit(c, o, 45, 40, 1, 0);
}


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *other = 0;
    FVM_Object *root = 0, *child = 0;
    FVM_Channel *ch = 0;
    FVM_Value args[3], out, value;
    FVM_Affine reflection = {-1, 0, 0, 1, 5, 0}, normal = {1, 0, 0, 1, 5, 0};
    FVM_Result opened;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &other) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_create(c, "root", 1, &root) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, root) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(args, "triangle");
    fvm_value_number(args + 1, 1);
    if (!(fvm_object_call(c, root, "createEmptyMovieClip", 2, args, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    child = out.object;
    fvm_value_number(&value, 20);
    if (!(fvm_object_set(c, root, "_x", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_set(c, root, "_y", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 5);
    if (!(fvm_object_set(c, child, "_x", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args, 0xff0000);
    if (!(fvm_object_call(c, child, "beginFill", 1, args, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    coordinate(c, child, "moveTo", 0, 0);
    coordinate(c, child, "lineTo", 10, 0);
    coordinate(c, child, "lineTo", 0, 10);
    if (!(fvm_object_call(c, child, "endFill", 0, 0, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    verify(c, root);
    fvm_value_number(args, 33);
    fvm_value_number(args + 1, 28);
    fvm_value_string(args + 2, "yes");
    if (!(fvm_object_call(c, root, "hitTest", 3, args, &out) == FVM_OK && !out.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    hit(c, root, 1e100, 0, 1, 0);
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = root;
    if (!(fvm_channel_create(&ch) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_send_value(c, ch, &value, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_receive_value(other, ch, &out, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(other, out.object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(other, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    verify(other, out.object);
    fvm_channel_destroy(ch);
    opened = fvm_channel_open_file("shape-hit-test.bin", "w", &ch);
    if (opened == FVM_OK)
    {
        if (!(fvm_channel_send_value(c, ch, &value, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_channel_destroy(ch);
        if (!(fvm_channel_open_file("shape-hit-test.bin", "r", &ch) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_channel_receive_value(other, ch, &out, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(other, out.object) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        verify(other, out.object);
        fvm_channel_destroy(ch);
        if (!(remove("shape-hit-test.bin") == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(opened == FVM_UNSUPPORTED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_movieclip_set_matrix(c, child, &reflection) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    hit(c, root, 23, 22, 1, 1);
    hit(c, root, 17, 28, 0, 1);
    hit(c, root, 17, 28, 1, 0);
    if (!(fvm_movieclip_set_matrix(c, child, &normal) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, child, "clear", 0, 0, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args, 2);
    fvm_value_number(args + 1, 0xff0000);
    if (!(fvm_object_call(c, child, "lineStyle", 2, args, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    coordinate(c, child, "moveTo", 0, 0);
    coordinate(c, child, "lineTo", 10, 0);
    hit(c, root, 30, 20.8, 1, 1);
    hit(c, root, 30, 22, 1, 0);
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
