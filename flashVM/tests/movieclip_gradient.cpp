#define FVM_GRADIENT_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "swf_gradient.cpp"
#include "flashvm/table.h"
#include "flashvm/avm1.h"


static inline FVM_Object *array(FVM_Context *c, double first, double second)
{
    FVM_Value v[2];
    fvm_value_number(v, first);
    fvm_value_number(v + 1, second);
    FVM_Object *o = 0;
    if (!(fvm_construct(c, "Array", 2, v, &o) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, o) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return o;
}


static inline void set(FVM_Context *c, FVM_Object *o, const char *key, double n)
{
    FVM_Value v;
    fvm_value_number(&v, n);
    if (!(fvm_object_set(c, o, key, &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static inline void ref(FVM_Value *value, FVM_Object *o)
{
    fvm_value_undefined(value);
    value->type = FVM_VALUE_OBJECT;
    value->object = o;
}


static inline void arguments(FVM_Context *c, FVM_Value *a, bool box)
{
    fvm_value_string(a, "linear");
    ref(a + 1, array(c, 0xff0000, 0x0000ff));
    ref(a + 2, array(c, 100, 100));
    ref(a + 3, array(c, 0, 255));
    FVM_Object *matrix = 0;
    if (!(fvm_object_create(c, &matrix) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, matrix) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (box)
    {
        FVM_Value type;
        fvm_value_string(&type, "box");
        if (!(fvm_object_set(c, matrix, "matrixType", &type) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        set(c, matrix, "w", 16);
        set(c, matrix, "h", 16);
    }
    else
    {
        set(c, matrix, "a", 16 / 1638.4);
        set(c, matrix, "d", 16 / 1638.4);
        set(c, matrix, "b", 0);
        set(c, matrix, "c", 0);
        set(c, matrix, "tx", 8);
        set(c, matrix, "ty", 8);
    }
    ref(a + 4, matrix);
    fvm_value_string(a + 5, "pad");
    fvm_value_string(a + 6, "RGB");
    fvm_value_number(a + 7, 0);
}


static inline FVM_Object *new_child(FVM_Context *c, FVM_Object *root, const char *name, unsigned int depth)
{
    FVM_Value a[2], out;
    fvm_value_string(a, name);
    fvm_value_number(a + 1, depth);
    if (!(fvm_object_call(c, root, "createEmptyMovieClip", 2, a, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return out.object;
}


static inline void geometry(FVM_Context *c, FVM_Object *o, bool finish = true)
{
    const double xy[] = {0, 0, 16, 0, 16, 16, 0, 16, 0, 0};
    FVM_Value a[2], out;
    for (unsigned int i = 0; i < 10; i += 2)
    {
        fvm_value_number(a, xy[i]);
        fvm_value_number(a + 1, xy[i + 1]);
        if (!(fvm_object_call(c, o, i ? "lineTo" : "moveTo", 2, a, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (finish)
    {
        if (!(fvm_object_call(c, o, "endFill", 0, 0, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
}
#ifndef FVM_MOVIECLIP_GRADIENT_FIXTURE_ONLY


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *other = 0;
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
    FVM_Object *root = 0;
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
    FVM_Value args[8], out;
    FVM_Object *script_clip = new_child(c, root, "script", 12);
    arguments(c, args, true);
    FVM_Value script_value;
    ref(&script_value, script_clip);
    if (!(fvm_set_global(c, "paintClip", &script_value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char *names[] = {"paintColors", "paintAlphas", "paintRatios", "paintMatrix"};
    for (unsigned int i = 0; i < 4; ++i)
    {
        if (!(fvm_set_global(c, names[i], args + i + 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    Bytes code;
    for (unsigned int i = 4; i; --i)
    {
        code.push_back(0x96);
        u16(code, (unsigned int)std::strlen(names[i - 1]) + 2);
        code.push_back(0);
        text(code, names[i - 1]);
        code.push_back(0x1c);
    }
    code.push_back(0x96);
    u16(code, 8);
    code.push_back(0);
    text(code, "linear");
    code.push_back(0x96);
    u16(code, 5);
    code.push_back(7);
    u32(code, 5);
    code.push_back(0x96);
    u16(code, 11);
    code.push_back(0);
    text(code, "paintClip");
    code.push_back(0x1c);
    code.push_back(0x96);
    u16(code, 19);
    code.push_back(0);
    text(code, "beginGradientFill");
    code.push_back(0x52);
    code.push_back(0x17);
    code.push_back(0);
    if (!(fvm_avm1_execute(c, code.begin(), code.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    geometry(c, script_clip);
    gradient_pixels(c, script_clip, false);
    for (unsigned int variant = 0; variant < 2; ++variant)
    {
        FVM_Object *clip = new_child(c, root, variant ? "matrix" : "box", variant + 1);
        arguments(c, args, variant == 0);
        if (!(fvm_object_call(c, clip, "beginGradientFill", 8, args, &out) == FVM_OK &&
              out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *data = clip->resource_result.object, *gradient = data->properties["gradient"].object;
        if (!(gradient && gradient != args[4].object))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value changed;
        fvm_value_number(&changed, 0);
        if (!(fvm_table_set_index(c, args[1].object, 0, &changed) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        set(c, args[4].object, variant ? "tx" : "w", 900);
        geometry(c, clip);
        if (!(data->properties.find("gradient") == data->properties.end()))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        gradient_pixels(c, clip, false);
        for (unsigned int i = 1; i <= 4; ++i)
        {
            if (!(fvm_gc_unpin(c, args[i].object) == FVM_OK))
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
        gradient_pixels(c, clip, false);
        FVM_Value value;
        ref(&value, clip);
        FVM_Channel *ch = 0;
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
        FVM_Object *received = out.object;
        if (!(fvm_gc_pin(other, received) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(other, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        gradient_pixels(other, received, false);
        fvm_channel_destroy(ch);
        if (!(fvm_standard_install(other) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value received_args[8];
        arguments(other, received_args, true);
        if (!(fvm_object_call(other, received, "beginGradientFill", 5, received_args, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        geometry(other, received);
        gradient_pixels(other, received, false);
        FVM_Channel *file = 0;
        FVM_Result opened = fvm_channel_open_file("flashvm-as-gradient-test.bin", "w", &file);
        if (opened == FVM_OK)
        {
            if (!(fvm_channel_send_value(c, file, &value, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_channel_destroy(file);
            if (!(fvm_channel_open_file("flashvm-as-gradient-test.bin", "r", &file) == FVM_OK))
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
            if (!(std::remove("flashvm-as-gradient-test.bin") == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            received = out.object;
            if (!(fvm_gc_pin(other, received) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(other, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            gradient_pixels(other, received, false);
        }
        else
        {
            if (!(opened == FVM_UNSUPPORTED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    FVM_Object *clip = new_child(c, root, "active", 3);
    arguments(c, args, true);
    if (!(fvm_object_call(c, clip, "beginGradientFill", 5, args, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    geometry(c, clip, false);
    FVM_Object *original = clip->resource_result.object->properties["gradient"].object;
    FVM_Value duplicate_args[2];
    fvm_value_string(duplicate_args, "copy");
    fvm_value_number(duplicate_args + 1, 4);
    if (!(fvm_object_call(c, clip, "duplicateMovieClip", 2, duplicate_args, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *copy = out.object;
    FVM_Object *copy_data = copy->resource_result.object, *cloned = copy_data->properties["gradient"].object;
    if (!(cloned && cloned != original))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *active = copy_data->properties["active"].object;
    if (!(active && active->properties["gradient"].object == cloned))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    gradient_pixels(c, copy, false);
    FVM_Value line[2];
    fvm_value_number(line, 1);
    fvm_value_number(line + 1, 0);
    if (!(fvm_object_call(c, clip, "lineStyle", 2, line, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(clip->resource_result.object->properties["gradient"].object == original))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value invalid[8];
    for (unsigned int i = 0; i < 8; ++i)
    {
        invalid[i] = args[i];
    }
    fvm_value_string(invalid, "unknown");
    if (!(fvm_object_call(c, clip, "beginGradientFill", 5, invalid, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(clip->resource_result.object->properties["gradient"].object == original))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(invalid, "linear");
    fvm_value_string(invalid + 5, "unknown-spread");
    if (!(fvm_object_call(c, clip, "beginGradientFill", 6, invalid, &out) == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(clip->resource_result.object->properties["gradient"].object == original))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    set(c, args[3].object, "length", 1);
    if (!(fvm_object_call(c, clip, "beginGradientFill", 5, args, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(clip->resource_result.object->properties["gradient"].object == original))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    set(c, args[3].object, "length", 2);
    FVM_Value ratio_end;
    fvm_value_number(&ratio_end, 255);
    if (!(fvm_table_set_index(c, args[3].object, 1, &ratio_end) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, clip, "clear", 0, 0, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *data = clip->resource_result.object;
    if (!(data->properties["fill"].number == 0 && data->properties["stroke"].number == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(data->properties.find("gradient") == data->properties.end()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    gradient_pixels(c, copy, false);
    if (!(fvm_object_call(c, clip, "beginGradientFill", 5, args, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(data->properties.find("gradient") != data->properties.end() && data->properties["fill"].number == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, clip, "beginGradientFill", 0, 0, &out) == FVM_OK && data->properties["fill"].number == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(data->properties.find("gradient") == data->properties.end()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    set(c, args[4].object, "w", 16);
    set(c, args[4].object, "h", 32);
    set(c, args[4].object, "r", std::acos(-1.0) / 2);
    if (!(fvm_object_call(c, clip, "beginGradientFill", 5, args, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *rotated = data->properties["gradient"].object;
    num(c, rotated, "yx", 32 / 1638.4);
    num(c, rotated, "xy", -16 / 1638.4);
    num(c, rotated, "x0", 8);
    num(c, rotated, "y0", 16);
    fvm_value_number(line, -0.5);
    if (!(fvm_object_call(c, clip, "beginFill", 1, line, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(data->properties["rgb"].number == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    set(c, root, "_alpha", 50);
    unsigned char pixels[16 * 16 * 4];
    std::memset(pixels, 0, sizeof(pixels));
    FVM_RasterSurface surface = {pixels, 16, 16, 64};
    FVM_Result rendered = fvm_movieclip_render(c, copy, &surface);
    if (rendered == FVM_OK)
    {
        unsigned char *p = pixels + 4 * (8 * 16 + 8);
        if (!(p[0] == 120 && p[2] == 135 && p[3] == 128))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(rendered == FVM_UNSUPPORTED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
#endif
