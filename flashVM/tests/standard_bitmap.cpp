#define FVM_GEOMETRY_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "standard_geometry.cpp"
#include "flashvm/serialization.h"
#include "../src/standard/bitmap_state.hpp"


static FVM_Object *bitmap(FVM_Context *c, double w, double h, bool transparent, unsigned long color)
{
    FVM_Value args[4];
    fvm_value_number(args, w);
    fvm_value_number(args + 1, h);
    fvm_value_boolean(args + 2, transparent);
    fvm_value_number(args + 3, (double)color);
    FVM_Object *o = 0;
    if (!(fvm_construct(c, "flash.display.BitmapData", 4, args, &o) == FVM_OK))
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


static double read(FVM_Context *c, FVM_Object *o, double x, double y, const char *method = "getPixel32")
{
    FVM_Value args[2];
    fvm_value_number(args, x);
    fvm_value_number(args + 1, y);
    double result = call(c, o, method, 2, args).number;
    return result < 0 ? result + 4294967296.0 : result;
}


static void
write(FVM_Context *c, FVM_Object *o, double x, double y, unsigned long color, const char *method = "setPixel32")
{
    FVM_Value args[3];
    fvm_value_number(args, x);
    fvm_value_number(args + 1, y);
    fvm_value_number(args + 2, (double)color);
    call(c, o, method, 3, args);
}


static void received(FVM_Context *c, FVM_Object *o)
{
    near(get(c, o, "width"), 3);
    near(get(c, o, "height"), 2);
    if (!(read(c, o, 1, 1) == 0x80443322UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(read(c, o, 1, 1) == 0x80443322UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value clone = call(c, o, "clone");
    if (!(fvm_gc_pin(c, clone.object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    write(c, clone.object, 1, 1, 0xffaabbccUL);
    if (!(read(c, o, 1, 1) == 0x80443322UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    call(c, clone.object, "dispose");
    near(get(c, clone.object, "width"), -1);
}
#ifndef FVM_BITMAP_FIXTURE_ONLY


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
    FVM_Object *b = bitmap(c, 3, 2, true, 0xff112233UL);
    near(get(c, b, "width"), 3);
    near(get(c, b, "height"), 2);
    if (!(read(c, b, 0, 0) == 0xff112233UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(read(c, b, -1, 0) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    write(c, b, 1, 1, 0x80443322UL);
    if (!(read(c, b, 1, 1) == 0x80443322UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(read(c, b, 1, 1, "getPixel") == 0x443322UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    write(c, b, 1, 1, 0x00998877UL, "setPixel");
    if (!(read(c, b, 1, 1) == 0x80998877UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    write(c, b, 1, 1, 0x80443322UL);
    FVM_Value signed_args[2];
    fvm_value_number(signed_args, 1);
    fvm_value_number(signed_args + 1, 1);
    if (!(call(c, b, "getPixel32", 2, signed_args).number == (double)0x80443322UL - 4294967296.0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *scrolling = bitmap(c, 3, 2, true, 0);
    write(c, scrolling, 0, 0, 0xff010203UL);
    write(c, scrolling, 1, 0, 0xff040506UL);
    write(c, scrolling, 2, 0, 0xff070809UL);
    FVM_Value shift[2];
    fvm_value_number(shift, 1);
    fvm_value_number(shift + 1, 0);
    call(c, scrolling, "scroll", 2, shift);
    if (!(read(c, scrolling, 0, 0) == 0xff010203UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(read(c, scrolling, 1, 0) == 0xff010203UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(read(c, scrolling, 2, 0) == 0xff040506UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(shift, -1);
    call(c, scrolling, "scroll", 2, shift);
    if (!(read(c, scrolling, 0, 0) == 0xff010203UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(read(c, scrolling, 1, 0) == 0xff040506UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(read(c, scrolling, 2, 0) == 0xff040506UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(shift, 3);
    call(c, scrolling, "scroll", 2, shift);
    if (!(read(c, scrolling, 1, 0) == 0xff040506UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *opaque = bitmap(c, 2, 2, false, 0x00112233);
    if (!(read(c, opaque, 0, 0) == 0xff112233UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    write(c, opaque, 0, 0, 0x00445566);
    if (!(read(c, opaque, 0, 0) == 0xff445566UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value rectangle;
    if (!(fvm_object_get(c, b, "rectangle", &rectangle) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, rectangle.object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    near(get(c, rectangle.object, "width"), 3);
    FVM_Value changed;
    fvm_value_number(&changed, 99);
    if (!(fvm_object_set(c, b, "width", &changed) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    near(get(c, b, "width"), 3);
    double area[] = {-1, -1, 2, 2};
    FVM_Object *r = make(c, "flash.geom.Rectangle", 4, area);
    FVM_Value args[2];
    args[0] = ref(r);
    fvm_value_number(args + 1, 0x12345678);
    call(c, b, "fillRect", 2, args);
    if (!(read(c, b, 0, 0) == 0x12345678))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(read(c, b, 1, 1) == 0x80443322UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    double transform_values[] = {2, 0.5, 1, 0, 10, -100, 300, 64};
    FVM_Value color_args[8];
    for (unsigned i = 0; i < 8; ++i)
    {
        fvm_value_number(color_args + i, transform_values[i]);
    }
    FVM_Object *ct = 0;
    if (!(fvm_construct(c, "flash.geom.ColorTransform", 8, color_args, &ct) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, ct) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    double transform_box[] = {0, 0, 1, 1};
    FVM_Object *tr = make(c, "flash.geom.Rectangle", 4, transform_box);
    FVM_Object *colored = bitmap(c, 2, 1, true, 0x80402010UL);
    FVM_Value transform_args[2] = {ref(tr), ref(ct)};
    call(c, colored, "colorTransform", 2, transform_args);
    if (!(read(c, colored, 0, 0) == 0x408a00ffUL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(read(c, colored, 1, 0) == 0x80402010UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    double copy_box[] = {0, 0, 2, 1}, copy_point[] = {1, 0};
    FVM_Object *cr = make(c, "flash.geom.Rectangle", 4, copy_box), *cp = make(c, "flash.geom.Point", 2, copy_point);
    FVM_Object *copy = bitmap(c, 3, 1, true, 0xff000000UL);
    write(c, copy, 0, 0, 0xff110000UL);
    write(c, copy, 1, 0, 0xff220000UL);
    FVM_Value ca[5];
    ca[0] = ref(copy);
    ca[1] = ref(cr);
    ca[2] = ref(cp);
    fvm_value_number(ca + 3, 1);
    fvm_value_number(ca + 4, 1);
    call(c, copy, "copyChannel", 5, ca);
    if (!(read(c, copy, 1, 0) == 0xff110000UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(read(c, copy, 2, 0) == 0xff220000UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(ca + 4, 2);
    call(c, copy, "copyChannel", 5, ca);
    if (!(read(c, copy, 1, 0) == 0xff111100UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(read(c, copy, 2, 0) == 0xff221100UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    double clip_box[] = {-1, 0, 3, 1}, clip_point[] = {0, 0};
    FVM_Object *clip_rect = make(c, "flash.geom.Rectangle", 4, clip_box),
               *clip_dest = make(c, "flash.geom.Point", 2, clip_point);
    FVM_Object *target = bitmap(c, 3, 1, false, 0xff000000UL);
    ca[0] = ref(copy);
    ca[1] = ref(clip_rect);
    ca[2] = ref(clip_dest);
    fvm_value_number(ca + 3, 1);
    fvm_value_number(ca + 4, 4);
    call(c, target, "copyChannel", 5, ca);
    if (!(read(c, target, 0, 0) == 0xff000000UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(read(c, target, 1, 0) == 0xff000011UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(read(c, target, 2, 0) == 0xff000011UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(ca + 4, 8);
    call(c, target, "copyChannel", 5, ca);
    if (!(read(c, target, 1, 0) == 0xff000011UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(ca + 4, 3);
    call(c, target, "copyChannel", 5, ca);
    if (!(read(c, target, 1, 0) == 0xff000011UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Channel *ch = 0;
    FVM_Value packet = ref(b), out;
    if (!(fvm_channel_create(&ch) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_send_value(c, ch, &packet, 0) == FVM_OK))
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
    fvm_channel_destroy(ch);
    received(other, out.object);
    unsigned char *data = 0;
    size_t size = 0;
    if (!(fvm_value_serialize(c, &packet, &data, &size) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t offset = size;
    for (size_t i = 0; i + 12 <= size; ++i)
    {
        if (std::memcmp(data + i, "FBM1", 4) == 0)
        {
            offset = i;
            break;
        }
    }
    if (!(offset < size))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned char saved = data[offset + 10];
    data[offset + 10] = 1;
    if (!(fvm_value_deserialize(other, data, size, &out) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    data[offset + 10] = saved;
    if (!(fvm_value_deserialize(other, data, size, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(other, out.object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    received(other, out.object);
    fvm_serialized_free(data);
    FVM_Result opened = fvm_channel_open_file("bitmap-test.bin", "w", &ch);
    if (opened == FVM_OK)
    {
        if (!(fvm_channel_send_value(c, ch, &packet, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_channel_destroy(ch);
        if (!(fvm_channel_open_file("bitmap-test.bin", "r", &ch) == FVM_OK))
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
        fvm_channel_destroy(ch);
        received(other, out.object);
        if (!(std::remove("bitmap-test.bin") == 0))
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
    call(c, b, "dispose");
    near(get(c, b, "width"), -1);
    packet = ref(b);
    if (!(fvm_value_serialize(c, &packet, &data, &size) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_deserialize(other, data, size, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    near(get(other, out.object, "width"), -1);
    fvm_serialized_free(data);
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
#endif
