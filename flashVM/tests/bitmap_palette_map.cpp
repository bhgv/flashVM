#define FVM_MERGE_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "bitmap_merge.cpp"
#include "flashvm/table.h"


static inline FVM_Object *palette(FVM_Context *c)
{
    FVM_Value length;
    fvm_value_number(&length, 256);
    FVM_Object *array = 0;
    if (!(fvm_construct(c, "Array", 1, &length, &array) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, array) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return array;
}


static inline void palette_entry(FVM_Context *c, FVM_Object *array, unsigned index, double number)
{
    FVM_Value v;
    fvm_value_number(&v, number);
    if (!(fvm_table_set_index(c, array, index, &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static inline void palette_args(FVM_Value *a,
                                FVM_Object *source,
                                FVM_Object *rect,
                                FVM_Object *point,
                                FVM_Object *red = 0,
                                FVM_Object *green = 0,
                                FVM_Object *blue = 0,
                                FVM_Object *alpha = 0)
{
    a[0] = ref(source);
    a[1] = ref(rect);
    a[2] = ref(point);
    FVM_Object *tables[] = {red, green, blue, alpha};
    for (unsigned i = 0; i < 4; ++i)
    {
        fvm_value_undefined(a + i + 3);
        if (tables[i])
        {
            a[i + 3] = ref(tables[i]);
        }
    }
}


static FVM_Result palette_dispose(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *u)
{
    FVM_Value ignored;
    if (!(fvm_object_call(c, (FVM_Object *)u, "dispose", 0, 0, &ignored) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, 0);
    return FVM_OK;
}
#ifndef FVM_PALETTE_FIXTURE_ONLY


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
    double box[] = {0, 0, 2, 1}, xy[] = {0, 0};
    FVM_Object *rect = region_object(c, "flash.geom.Rectangle", 4, box),
               *point = region_object(c, "flash.geom.Point", 2, xy);
    FVM_Object *source = bitmap(c, 2, 1, true, 0xffff0000UL);
    write(c, source, 1, 0, 0xff00ff00UL);
    FVM_Object *red = palette(c), *green = palette(c), *dest = bitmap(c, 2, 1, true, 0);
    palette_entry(c, red, 255, 0xff00);
    palette_entry(c, green, 255, 0xff0000);
    FVM_Value a[7];
    palette_args(a, source, rect, point, red, green);
    call(c, dest, "paletteMap", 7, a);
    if (!(read(c, dest, 0, 0) == 0xff00ff00UL && read(c, dest, 1, 0) == 0xffff0000UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    palette_args(a, source, rect, point);
    call(c, dest, "paletteMap", 3, a);
    if (!(read(c, dest, 0, 0) == 0xffff0000UL && read(c, dest, 1, 0) == 0xff00ff00UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *one = bitmap(c, 1, 1, true, 0x00010203UL);
    double one_box[] = {0, 0, 1, 1};
    FVM_Object *one_rect = region_object(c, "flash.geom.Rectangle", 4, one_box);
    FVM_Object *r = palette(c), *g = palette(c), *b = palette(c), *alpha = palette(c);
    palette_entry(c, r, 1, -1);
    palette_entry(c, g, 2, 2);
    palette_args(a, one, one_rect, point, r, g, b, alpha);
    call(c, one, "paletteMap", 7, a);
    if (!(read(c, one, 0, 0) == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    palette_entry(c, r, 0, std::numeric_limits<double>::infinity());
    palette_args(a, one, one_rect, point, r, g, b, alpha);
    call(c, one, "paletteMap", 7, a);
    if (!(read(c, one, 0, 0) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *opaque = bitmap(c, 2, 1, false, 0);
    palette_args(a, source, rect, point, red, green, b, alpha);
    call(c, opaque, "paletteMap", 7, a);
    if (!(read(c, opaque, 0, 0) == 0xff00ff00UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *alias = bitmap(c, 4, 1, true, 1);
    for (unsigned x = 0; x < 4; ++x)
    {
        write(c, alias, x, 0, x + 1);
    }
    double three[] = {0, 0, 3, 1}, shift[] = {1, 0};
    FVM_Object *three_rect = region_object(c, "flash.geom.Rectangle", 4, three),
               *shift_point = region_object(c, "flash.geom.Point", 2, shift);
    palette_args(a, alias, three_rect, shift_point);
    call(c, alias, "paletteMap", 3, a);
    if (!(read(c, alias, 0, 0) == 1 && read(c, alias, 1, 0) == 1 && read(c, alias, 2, 0) == 2 &&
          read(c, alias, 3, 0) == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Channel *ch = 0;
    if (!(fvm_channel_create(&ch) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *objects[] = {source, rect, point, red, green};
    FVM_Value copied[5];
    for (unsigned i = 0; i < 5; ++i)
    {
        FVM_Value value = ref(objects[i]);
        if (!(fvm_channel_send_value(c, ch, &value, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_channel_receive_value(other, ch, copied + i, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(other, copied[i].object) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_gc_collect(other, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    palette_args(a, copied[0].object, copied[1].object, copied[2].object, copied[3].object, copied[4].object);
    call(other, copied[0].object, "paletteMap", 7, a);
    if (!(read(other, copied[0].object, 0, 0) == 0xff00ff00UL && read(other, copied[0].object, 1, 0) == 0xffff0000UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_channel_destroy(ch);
    palette_args(a, source, rect, point, red, green);
    unsigned long previous = rt->config.memory_limit_bytes;
    rt->config.memory_limit_bytes = 1;
    FVM_Value out;
    if (!(fvm_object_call(c, dest, "paletteMap", 7, a, &out) == FVM_OUT_OF_MEMORY))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    rt->config.memory_limit_bytes = previous;
    if (!(read(c, dest, 0, 0) == 0xffff0000UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *disposed = bitmap(c, 2, 1, true, 0), *callback_table = 0;
    if (!(fvm_object_create(c, &callback_table) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, callback_table) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_accessor_rooted(c, callback_table, "0", palette_dispose, 0, disposed, disposed) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    palette_args(a, source, rect, point, callback_table);
    if (!(call(c, disposed, "paletteMap", 7, a).number == -1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_bitmap_pixels(disposed)->disposed))
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
