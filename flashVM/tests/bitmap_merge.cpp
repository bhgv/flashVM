#define FVM_THRESHOLD_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "bitmap_threshold.cpp"
#include <limits>


static inline void
merge_args(FVM_Value *a, FVM_Object *source, FVM_Object *rect, FVM_Object *point, const double *weights)
{
    a[0] = ref(source);
    a[1] = ref(rect);
    a[2] = ref(point);
    for (unsigned i = 0; i < 4; ++i)
    {
        fvm_value_number(a + i + 3, weights[i]);
    }
}
#ifndef FVM_MERGE_FIXTURE_ONLY


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
    FVM_Object *source = bitmap(c, 2, 1, true, 0x80c86432UL);
    write(c, source, 1, 0, 0x40ff0000UL);
    FVM_Object *dest = bitmap(c, 2, 1, true, 0xc864c8faUL);
    double half[] = {128, 128, 128, 128};
    FVM_Value a[7];
    merge_args(a, source, rect, point, half);
    if (!(call(c, dest, "merge", 7, a).type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(read(c, dest, 0, 0) == 0xa4969696UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(read(c, dest, 1, 0) == 0x84b1647dUL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    double selected[] = {256, 0, 256, 0};
    dest = bitmap(c, 2, 1, true, 0xc864c8faUL);
    merge_args(a, source, rect, point, selected);
    call(c, dest, "merge", 7, a);
    if (!(read(c, dest, 0, 0) == 0xc8c8c832UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    double zero[] = {0, 0, 0, 0};
    merge_args(a, source, rect, point, zero);
    unsigned long previous = rt->config.memory_limit_bytes;
    rt->config.memory_limit_bytes = 1;
    call(c, dest, "merge", 7, a);
    rt->config.memory_limit_bytes = previous;
    if (!(read(c, dest, 0, 0) == 0xc8c8c832UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    double clamped[] = {-1, 300, 255.9, 256};
    dest = bitmap(c, 2, 1, true, 0xc864c8faUL);
    merge_args(a, source, rect, point, clamped);
    call(c, dest, "merge", 7, a);
    if (!(read(c, dest, 0, 0) == 0x80646432UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *opaque = bitmap(c, 2, 1, false, 0xff0000ffUL);
    double all[] = {256, 256, 256, 256};
    merge_args(a, source, rect, point, all);
    call(c, opaque, "merge", 7, a);
    if (!(read(c, opaque, 0, 0) == 0xffc86432UL))
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
    merge_args(a, alias, three_rect, shift_point, all);
    call(c, alias, "merge", 7, a);
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
    FVM_Value value = ref(source), copied_source, copied_rect, copied_point;
    if (!(fvm_channel_send_value(c, ch, &value, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_receive_value(other, ch, &copied_source, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(other, copied_source.object) == FVM_OK))
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
    merge_args(a, copied_source.object, copied_rect.object, copied_point.object, half);
    call(other, copied_source.object, "merge", 7, a);
    if (!(read(other, copied_source.object, 0, 0) == 0x80c86432UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_channel_destroy(ch);
    merge_args(a, source, rect, point, half);
    rt->config.memory_limit_bytes = 1;
    FVM_Value out;
    if (!(fvm_object_call(c, dest, "merge", 7, a, &out) == FVM_OUT_OF_MEMORY))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    rt->config.memory_limit_bytes = previous;
    if (!(read(c, dest, 0, 0) == 0x80646432UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(a + 3, std::numeric_limits<double>::infinity());
    if (!(fvm_object_call(c, dest, "merge", 7, a, &out) == FVM_INVALID_ARGUMENT))
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
