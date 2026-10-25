#include <stdlib.h>
#include "flashvm/movieclip.h"
#include "flashvm/execution.h"
#include "flashvm/standard.h"
#include "flashvm/transfer.h"
#include "flashvm/gc.h"
#include <string.h>
#include <stdio.h>


static FVM_Value get(FVM_Context *c, FVM_Object *o, const char *key)
{
    FVM_Value out;
    if (!(fvm_object_get(c, o, key, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return out;
}


static void numeric(FVM_Context *c, FVM_Object *o, const char *key, double n)
{
    FVM_Value value = get(c, o, key);
    if (!(value.type == FVM_VALUE_NUMBER && value.number == n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void coordinate(FVM_Context *c, FVM_Object *o, const char *method, double x, double y)
{
    FVM_Value args[2], out;
    fvm_value_number(args, x);
    fvm_value_number(args + 1, y);
    if (!(fvm_object_call(c, o, method, 2, args, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static FVM_Object *duplicate(FVM_Context *c, FVM_Object *o, const char *name, double depth, FVM_Object *initial)
{
    FVM_Value args[3], out;
    fvm_value_string(args, name);
    fvm_value_number(args + 1, depth);
    fvm_value_undefined(args + 2);
    if (initial)
    {
        args[2].type = FVM_VALUE_OBJECT;
        args[2].object = initial;
    }
    if (!(fvm_exec_duplicate_clip(c, o, args, args + 1, initial ? args + 2 : 0, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return out.type == FVM_VALUE_OBJECT ? out.object : 0;
}


static void bounds(FVM_Context *c, FVM_Object *o, double x, double y)
{
    FVM_Value out;
    if (!(fvm_object_call(c, o, "getBounds", 0, 0, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    numeric(c, out.object, "xMax", x);
    numeric(c, out.object, "yMax", y);
}


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *other = 0;
    FVM_Object *root = 0, *source = 0, *copy = 0, *initial = 0, *initialized = 0, *same_depth = 0;
    FVM_Value args[3], out, value;
    FVM_Channel *ch = 0;
    unsigned char original_pixels[32 * 32 * 4], copied_pixels[32 * 32 * 4];
    FVM_RasterSurface a, b;
    FVM_Result ra, rb;
    size_t i;
    int colored = 0;
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
    fvm_value_string(args, "source");
    fvm_value_number(args + 1, 1);
    if (!(fvm_object_call(c, root, "createEmptyMovieClip", 2, args, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    source = out.object;
    fvm_value_number(&value, 4);
    if (!(fvm_object_set(c, source, "_x", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 5);
    if (!(fvm_object_set(c, source, "_y", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 73);
    if (!(fvm_object_set(c, source, "custom", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args, 0xff0000);
    fvm_value_number(args + 1, 100);
    if (!(fvm_object_call(c, source, "beginFill", 2, args, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    coordinate(c, source, "moveTo", 0, 0);
    coordinate(c, source, "lineTo", 10, 0);
    coordinate(c, source, "lineTo", 10, 10);
    coordinate(c, source, "lineTo", 0, 10);
    coordinate(c, source, "lineTo", 0, 0);
    if (!(fvm_object_call(c, source, "stop", 0, 0, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(args, "runtimeChild");
    fvm_value_number(args + 1, 9);
    if (!(fvm_object_call(c, source, "createEmptyMovieClip", 2, args, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    copy = duplicate(c, source, "copy", 2, 0);
    if (!(copy && copy != source))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    numeric(c, copy, "_x", 4);
    numeric(c, copy, "_y", 5);
    numeric(c, copy, "_currentframe", 1);
    if (!(get(c, copy, "custom").type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(get(c, copy, "_parent").object == root))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(get(c, copy, "runtimeChild").type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args, 9);
    if (!(fvm_object_call(c, copy, "getInstanceAtDepth", 1, args, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bounds(c, source, 10, 10);
    bounds(c, copy, 10, 10);
    memset(original_pixels, 0, sizeof(original_pixels));
    memset(copied_pixels, 0, sizeof(copied_pixels));
    a.pixels = original_pixels;
    a.width = 32;
    a.height = 32;
    a.stride = 32 * 4;
    b = a;
    b.pixels = copied_pixels;
    ra = fvm_movieclip_render(c, source, &a);
    rb = fvm_movieclip_render(c, copy, &b);
    if (!(ra == rb && (ra == FVM_OK || ra == FVM_UNSUPPORTED)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (ra == FVM_OK)
    {
        if (!(!memcmp(original_pixels, copied_pixels, sizeof(original_pixels))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (i = 0; i < sizeof(original_pixels); ++i)
        {
            if (original_pixels[i])
            {
                colored = 1;
            }
        }
        if (!(colored))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    coordinate(c, source, "lineTo", 20, 0);
    bounds(c, source, 20, 10);
    bounds(c, copy, 10, 10);
    coordinate(c, copy, "lineTo", 0, 20);
    bounds(c, copy, 10, 20);
    bounds(c, source, 20, 10);
    if (!(fvm_object_call(c, source, "clear", 0, 0, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bounds(c, source, 0, 0);
    bounds(c, copy, 10, 20);
    if (!(fvm_object_create(c, &initial) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 8);
    if (!(fvm_object_set(c, initial, "_x", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_set(c, initial, "seed", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    initialized = duplicate(c, copy, "initialized", 3, initial);
    if (!(initialized))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    numeric(c, initialized, "_x", 8);
    numeric(c, initialized, "seed", 8);
    numeric(c, copy, "_x", 4);
    if (!(!duplicate(c, root, "rootCopy", 4, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    same_depth = duplicate(c, source, "replacement", 1, 0);
    if (!(same_depth))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(get(c, source, "_parent").type == FVM_VALUE_NULL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(get(c, root, "source").type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bounds(c, copy, 10, 20);
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = copy;
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
    copy = out.object;
    if (!(fvm_gc_pin(other, copy) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(other, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    initialized = duplicate(other, copy, "remoteDuplicate", 12, 0);
    if (!(initialized))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bounds(other, initialized, 10, 20);
    if (!(fvm_movieclip_advance(other, initialized, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(other, copy, "clear", 0, 0, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bounds(other, initialized, 10, 20);
    {
        FVM_Channel *file = 0;
        FVM_Result file_result = fvm_channel_open_file("flashvm-execution-duplicate-test.bin", "w", &file);
        if (file_result == FVM_OK)
        {
            if (!(fvm_channel_send_value(c, file, &value, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_channel_destroy(file);
            if (!(fvm_channel_open_file("flashvm-execution-duplicate-test.bin", "r", &file) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_channel_receive_value(other, file, &out, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_channel_destroy(file);
            if (!(remove("flashvm-execution-duplicate-test.bin") == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            copy = out.object;
            if (!(fvm_gc_pin(other, copy) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(other, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            initialized = duplicate(other, copy, "fileDuplicate", 13, 0);
            if (!(initialized))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            bounds(other, initialized, 10, 20);
        }
        else
        {
            if (!(file_result == FVM_UNSUPPORTED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    fvm_channel_destroy(ch);
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
