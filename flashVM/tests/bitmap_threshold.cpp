#define FVM_BITMAP_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "standard_bitmap.cpp"


static inline FVM_Object *region_object(FVM_Context *c, const char *name, unsigned n, const double *v)
{
    return make(c, name, n, v);
}


static inline void threshold_args(FVM_Value *a,
                                  FVM_Object *source,
                                  FVM_Object *rect,
                                  FVM_Object *point,
                                  const char *operation,
                                  double threshold,
                                  double color = 0xaa010203UL,
                                  double mask = 0xff000000UL,
                                  bool copy = false)
{
    a[0] = ref(source);
    a[1] = ref(rect);
    a[2] = ref(point);
    fvm_value_string(a + 3, operation);
    fvm_value_number(a + 4, threshold);
    fvm_value_number(a + 5, color);
    fvm_value_number(a + 6, mask);
    fvm_value_boolean(a + 7, copy);
}
#ifndef FVM_THRESHOLD_FIXTURE_ONLY


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
    FVM_Object *source = bitmap(c, 4, 1, true, 0x00112233UL);
    const unsigned long colors[] = {0x00112233UL, 0x7f112233UL, 0x80112233UL, 0xff112233UL};
    for (unsigned x = 0; x < 4; ++x)
    {
        write(c, source, x, 0, colors[x]);
    }
    double box[] = {0, 0, 4, 1}, xy[] = {0, 0};
    FVM_Object *rect = region_object(c, "flash.geom.Rectangle", 4, box),
               *point = region_object(c, "flash.geom.Point", 2, xy);
    const char *operators[] = {"<", "<=", ">", ">=", "==", "!="};
    const unsigned counts[] = {2, 3, 1, 2, 1, 3};
    const bool matches[][4] = {{true, true, false, false},
                               {true, true, true, false},
                               {false, false, false, true},
                               {false, false, true, true},
                               {false, false, true, false},
                               {true, true, false, true}};
    for (unsigned op = 0; op < 6; ++op)
    {
        for (unsigned copy = 0; copy < 2; ++copy)
        {
            FVM_Object *dest = bitmap(c, 4, 1, true, 0x40445566UL);
            FVM_Value a[8];
            threshold_args(a, source, rect, point, operators[op], 0x80000000UL, 0xaa010203UL, 0xff000000UL, copy != 0);
            FVM_Value out = call(c, dest, "threshold", 8, a);
            if (!(out.type == FVM_VALUE_NUMBER && out.number == counts[op]))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            for (unsigned x = 0; x < 4; ++x)
            {
                if (!(read(c, dest, x, 0) == (matches[op][x] ? 0xaa010203UL : copy ? colors[x] : 0x40445566UL)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    FVM_Object *opaque = bitmap(c, 4, 1, false, 0);
    FVM_Value a[8];
    threshold_args(a, source, rect, point, "==", 0x80000000UL, 0xaa010203UL, 0xff000000UL, true);
    if (!(call(c, opaque, "threshold", 8, a).number == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned x = 0; x < 4; ++x)
    {
        if (!(read(c, opaque, x, 0) == (x == 2 ? 0xff010203UL : colors[x] | 0xff000000UL)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    FVM_Object *dest = bitmap(c, 4, 1, true, 0xffffffffUL);
    threshold_args(a, source, rect, point, "<", 4294967295.0);
    if (!(call(c, dest, "threshold", 5, a).number == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned x = 0; x < 4; ++x)
    {
        if (!(read(c, dest, x, 0) == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    threshold_args(a, source, rect, point, "==", 999, 0x80445566UL, 0);
    if (!(call(c, dest, "threshold", 8, a).number == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(read(c, dest, 0, 0) == 0x80445566UL))
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
    threshold_args(a, alias, three_rect, shift_point, "==", 0, 0, 0xffffffffUL, true);
    if (!(call(c, alias, "threshold", 8, a).number == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(read(c, alias, 0, 0) == 1 && read(c, alias, 1, 0) == 1 && read(c, alias, 2, 0) == 2 &&
          read(c, alias, 3, 0) == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    double clipped_box[] = {-1, 0, 4, 1};
    FVM_Object *clipped = region_object(c, "flash.geom.Rectangle", 4, clipped_box);
    dest = bitmap(c, 4, 1, true, 0x40445566UL);
    threshold_args(a, source, clipped, point, "==", 0, 0xaa010203UL, 0);
    if (!(call(c, dest, "threshold", 8, a).number == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(read(c, dest, 0, 0) == 0x40445566UL && read(c, dest, 1, 0) == 0xaa010203UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    double negative[] = {-1, 0};
    FVM_Object *negative_point = region_object(c, "flash.geom.Point", 2, negative);
    dest = bitmap(c, 4, 1, true, 0x40445566UL);
    threshold_args(a, source, rect, negative_point, "==", 0, 0xaa010203UL, 0);
    if (!(call(c, dest, "threshold", 8, a).number == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(read(c, dest, 3, 0) == 0x40445566UL && read(c, dest, 0, 0) == 0xaa010203UL))
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
    FVM_Value value = ref(source), out;
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
    value = ref(rect);
    if (!(fvm_channel_send_value(c, ch, &value, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value copied_rect;
    if (!(fvm_channel_receive_value(other, ch, &copied_rect, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(other, copied_rect.object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    value = ref(point);
    if (!(fvm_channel_send_value(c, ch, &value, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value copied_point;
    if (!(fvm_channel_receive_value(other, ch, &copied_point, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(other, copied_point.object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(other, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    threshold_args(a, out.object, copied_rect.object, copied_point.object, "==", 0x80000000UL);
    if (!(call(other, out.object, "threshold", 8, a).number == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(read(other, out.object, 2, 0) == 0xaa010203UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_channel_destroy(ch);
    threshold_args(a, source, rect, point, "bad", 0);
    if (!(fvm_object_call(c, dest, "threshold", 8, a, &out) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(read(c, dest, 3, 0) == 0x40445566UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    threshold_args(a, source, rect, point, "==", 0);
    if (!(fvm_value_string_bytes(a + 3, "==\0junk", 7) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, dest, "threshold", 8, a, &out) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned long previous = rt->config.memory_limit_bytes;
    rt->config.memory_limit_bytes = 1;
    threshold_args(a, source, rect, point, "==", 0, 0, 0);
    if (!(fvm_object_call(c, dest, "threshold", 8, a, &out) == FVM_OUT_OF_MEMORY))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    rt->config.memory_limit_bytes = previous;
    if (!(read(c, dest, 3, 0) == 0x40445566UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    call(c, source, "dispose");
    if (!(call(c, dest, "threshold", 8, a).number == -1))
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
