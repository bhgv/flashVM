#define FVM_FLOOD_FILL_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "bitmap_flood_fill.cpp"


static inline FVM_Object *bounds(FVM_Context *c, FVM_Object *image, double mask, double color, bool find = true)
{
    FVM_Value a[3];
    fvm_value_number(a, mask);
    fvm_value_number(a + 1, color);
    fvm_value_boolean(a + 2, find);
    FVM_Value out = call(c, image, "getColorBoundsRect", 3, a);
    if (!(out.type == FVM_VALUE_OBJECT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, out.object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return out.object;
}


static inline void box(FVM_Context *c, FVM_Object *rect, double x, double y, double width, double height)
{
    near(get(c, rect, "x"), x);
    near(get(c, rect, "y"), y);
    near(get(c, rect, "width"), width);
    near(get(c, rect, "height"), height);
    if (!(rect->resource_kind == FVM_GEOMETRY_RECTANGLE))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}
#ifndef FVM_COLOR_BOUNDS_FIXTURE_ONLY


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
    FVM_Object *image = bitmap(c, 5, 4, true, 0xff0000ffUL);
    write(c, image, 1, 1, 0x80ff0000UL);
    write(c, image, 3, 2, 0xffff0000UL);
    box(c, bounds(c, image, 0xffffffUL, 0xff0000UL), 1, 1, 3, 2);
    box(c, bounds(c, image, 0xffffffffUL, 0x80ff0000UL), 1, 1, 1, 1);
    box(c, bounds(c, image, 0xffffffUL, 0xff0000UL, false), 0, 0, 5, 4);
    box(c, bounds(c, image, 0xff000000UL, 0), 0, 0, 0, 0);
    box(c, bounds(c, image, 0xff000000UL, 0, false), 0, 0, 5, 4);
    box(c, bounds(c, image, 0xff000000UL, 0xffff0000UL), 0, 0, 0, 0);
    box(c, bounds(c, image, 0, 0), 0, 0, 5, 4);
    box(c, bounds(c, image, 0, 0, false), 0, 0, 0, 0);
    write(c, image, 4, 3, 0x00ff0000UL);
    box(c, bounds(c, image, 0xffffffUL, 0xff0000UL), 1, 1, 4, 3);
    box(c, bounds(c, image, 0xff000000UL, 0), 4, 3, 1, 1);
    FVM_Value default_args[2];
    fvm_value_number(default_args, 0xffffffUL);
    fvm_value_number(default_args + 1, 0xff0000UL);
    FVM_Value default_out = call(c, image, "getColorBoundsRect", 2, default_args);
    box(c, default_out.object, 1, 1, 4, 3);
    FVM_Object *opaque = bitmap(c, 2, 2, false, 0xffffffffUL);
    box(c, bounds(c, opaque, -1, -1), 0, 0, 2, 2);
    FVM_Object *a = bounds(c, image, 0xffffffUL, 0xff0000UL), *b = bounds(c, image, 0xffffffUL, 0xff0000UL);
    if (!(a != b && a->prototype == b->prototype))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value changed;
    fvm_value_number(&changed, 99);
    if (!(fvm_object_set(c, a, "x", &changed) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    box(c, b, 1, 1, 4, 3);
    FVM_Channel *ch = 0;
    FVM_Value value = ref(image), out;
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
    box(other, bounds(other, out.object, 0xffffffUL, 0xff0000UL), 1, 1, 4, 3);
    fvm_channel_destroy(ch);
    FVM_Object *disposed = bitmap(c, 2, 2, true, 0);
    call(c, disposed, "dispose");
    FVM_Value invalid = call(c, disposed, "getColorBoundsRect", 2, default_args);
    if (!(invalid.type == FVM_VALUE_NUMBER && invalid.number == -1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(default_args, std::numeric_limits<double>::infinity());
    if (!(fvm_object_call(c, image, "getColorBoundsRect", 2, default_args, &invalid) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    box(c, b, 1, 1, 4, 3);
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
#endif
