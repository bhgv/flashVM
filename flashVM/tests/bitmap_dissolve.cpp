#define FVM_NOISE_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "bitmap_noise.cpp"


static inline FVM_Object *dissolve_rect(FVM_Context *c, double x, double y, double w, double h)
{
    FVM_Object *o = 0;
    if (!(fvm_object_create(c, &o) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, o) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    set(c, o, "x", x);
    set(c, o, "y", y);
    set(c, o, "width", w);
    set(c, o, "height", h);
    return o;
}


static inline void dissolve_args(FVM_Value *a,
                                 FVM_Object *source,
                                 FVM_Object *rect,
                                 FVM_Object *point,
                                 double seed = 0,
                                 double count = 3,
                                 double color = 0xffabcdefUL)
{
    a[0] = ref(source);
    a[1] = ref(rect);
    a[2] = ref(point);
    fvm_value_number(a + 3, seed);
    fvm_value_number(a + 4, count);
    fvm_value_number(a + 5, color);
}


static inline void dissolve_packet(FVM_Context *c, FVM_Object *image)
{
    if (!(fvm_gc_pin(c, image) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *point = 0;
    if (!(fvm_object_create(c, &point) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, point) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    set(c, point, "x", 100);
    set(c, point, "y", 100);
    FVM_Value a[6];
    dissolve_args(a, image, dissolve_rect(c, 0, 0, 4, 3), point, 0, 12, 0xffabcdefUL);
    call(c, image, "pixelDissolve", 6, a);
    for (unsigned y = 0; y < 3; ++y)
    {
        for (unsigned x = 0; x < 4; ++x)
        {
            if (!(read(c, image, x, y) == 0xffabcdefUL))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
}
#ifndef FVM_DISSOLVE_FIXTURE_ONLY


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
    FVM_Object *source = bitmap(c, 4, 3, true, 0xff123456UL), *dest = bitmap(c, 4, 3, true, 0xff000000UL),
               *rect = dissolve_rect(c, 0, 0, 4, 3), *point = hit_point(c, 0, 0);
    FVM_Value a[6], out;
    const unsigned order[] = {9, 6, 11, 0, 2, 1, 3, 8, 5, 10, 7, 4};
    const unsigned cursors[] = {1, 2, 3, 4, 6, 9, 11, 12, 13, 14, 15, 0};
    unsigned long random = c->random_state;
    double seed = 0;
    for (unsigned i = 0; i < 12; ++i)
    {
        dissolve_args(a, source, rect, point, seed, 1);
        seed = call(c, dest, "pixelDissolve", 6, a).number;
        if (!(seed == cursors[i]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned k = 0; k < 12; ++k)
        {
            bool selected = false;
            for (unsigned j = 0; j <= i; ++j)
            {
                selected |= k == order[j];
            }
            if (!(read(c, dest, k % 4, k / 4) == (selected ? 0xff123456UL : 0xff000000UL)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    if (!(c->random_state == random))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *all = bitmap(c, 4, 3, true, 0xff000000UL);
    dissolve_args(a, source, rect, point, 0, 1000);
    if (!(call(c, all, "pixelDissolve", 6, a).number == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned y = 0; y < 3; ++y)
    {
        for (unsigned x = 0; x < 4; ++x)
        {
            if (!(read(c, all, x, y) == read(c, dest, x, y)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    dissolve_args(a, dest, rect, hit_point(c, 1000, 1000), 0, 12, 0x80010203UL);
    call(c, dest, "pixelDissolve", 6, a);
    for (unsigned y = 0; y < 3; ++y)
    {
        for (unsigned x = 0; x < 4; ++x)
        {
            if (!(read(c, dest, x, y) == 0x80010203UL))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    FVM_Object *opaque = bitmap(c, 4, 3, false, 0);
    dissolve_args(a, opaque, rect, point, -1, 12, 0x00010203UL);
    call(c, opaque, "pixelDissolve", 6, a);
    if (!(read(c, opaque, 0, 0) == 0xff010203UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    dissolve_args(a, dest, rect, point, 0, 12, 0x00010203UL);
    call(c, dest, "pixelDissolve", 6, a);
    if (!(read(c, dest, 0, 0) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *clipped = bitmap(c, 4, 3, true, 0xff999999UL);
    dissolve_args(a, source, dissolve_rect(c, -1, 0, 4, 3), point, 0, 99);
    call(c, clipped, "pixelDissolve", 6, a);
    for (unsigned y = 0; y < 3; ++y)
    {
        if (!(read(c, clipped, 0, y) == 0xff999999UL))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned x = 1; x < 4; ++x)
        {
            if (!(read(c, clipped, x, y) == 0xff123456UL))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    dissolve_args(a, source, rect, point, -1, 0);
    if (!(call(c, clipped, "pixelDissolve", 6, a).number == 15))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    dissolve_args(a, source, rect, point, 4294967295.0, 0);
    if (!(call(c, clipped, "pixelDissolve", 6, a).number == 15))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    dissolve_args(a, source, rect, point, 0, -1);
    if (!(fvm_object_call(c, clipped, "pixelDissolve", 6, a, &out) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned long before = (unsigned long)read(c, clipped, 0, 0), limit = rt->config.memory_limit_bytes;
    rt->config.memory_limit_bytes = 95;
    dissolve_args(a, source, rect, point, 0, 3);
    if (!(fvm_object_call(c, clipped, "pixelDissolve", 6, a, &out) == FVM_OUT_OF_MEMORY))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    rt->config.memory_limit_bytes = limit;
    if (!(read(c, clipped, 0, 0) == before))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *small = bitmap(c, 1, 1, true, 0xff000000UL);
    dissolve_args(a, small, dissolve_rect(c, 0, 0, 1, 1), point, 0, 1);
    call(c, small, "pixelDissolve", 6, a);
    if (!(read(c, small, 0, 0) == 0xffabcdefUL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *defaulted = bitmap(c, 30, 2, true, 0xff000000UL);
    dissolve_args(a, defaulted, dissolve_rect(c, 0, 0, 30, 2), point);
    call(c, defaulted, "pixelDissolve", 3, a);
    unsigned changed = 0;
    for (unsigned y = 0; y < 2; ++y)
    {
        for (unsigned x = 0; x < 30; ++x)
        {
            changed += read(c, defaulted, x, y) == 0;
        }
    }
    if (!(changed == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Channel *ch = 0;
    FVM_Value value = ref(all), copy;
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
    if (!(fvm_channel_receive_value(other, ch, &copy, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    dissolve_packet(other, copy.object);
    fvm_channel_destroy(ch);
    if (!(fvm_channel_open_file("dissolve-test.bin", "w", &ch) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_send_value(c, ch, &value, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_channel_destroy(ch);
    if (!(fvm_channel_open_file("dissolve-test.bin", "r", &ch) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_receive_value(other, ch, &copy, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_channel_destroy(ch);
    dissolve_packet(other, copy.object);
    if (!(std::remove("dissolve-test.bin") == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#ifdef OS_PORTS
    FVM_ResourceBroker *broker = 0;
    FVM_Channel *queue = 0, *network = 0;
    FVM_Object *wrapped = 0;
    char *uri = 0;
    if (!(fvm_broker_start("127.0.0.1", "127.0.0.1", 0, &broker) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_set_broker(c, broker) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_create(&queue) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_extensions_wrap_channel(c, queue, &wrapped) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_broker_export(c, wrapped, &uri) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_open_network(other, uri, &network) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_send_value(c, queue, &value, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_receive_value(other, network, &copy, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    dissolve_packet(other, copy.object);
    fvm_channel_release(network);
    fvm_broker_stop(broker);
    fvm_channel_release(queue);
    fvm_broker_release(broker);
    fvm_serialized_free(uri);
#endif
    FVM_Object *disposed = bitmap(c, 4, 3, true, 0), *callback = 0;
    if (!(fvm_object_create(c, &callback) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, callback) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_method_rooted(c, callback, "valueOf", noise_dispose_seed, disposed, disposed) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    dissolve_args(a, source, rect, point);
    a[3] = ref(callback);
    if (!(call(c, disposed, "pixelDissolve", 6, a).number == -1))
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
