#define FVM_COLOR_FILTER_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "color_matrix_filter.cpp"
#include "../src/standard/convolution.hpp"


static inline FVM_Object *kernel_array(FVM_Context *c, unsigned n, const double *values)
{
    FVM_Value length;
    fvm_value_number(&length, n);
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
    for (unsigned i = 0; i < n; ++i)
    {
        palette_entry(c, array, i, values[i]);
    }
    return array;
}


static inline FVM_Object *convolution(FVM_Context *c,
                                      unsigned columns,
                                      unsigned rows,
                                      const double *values,
                                      double divisor = 1,
                                      double bias = 0,
                                      bool preserve = true,
                                      bool clamp = true,
                                      double color = 0,
                                      double alpha = 0)
{
    FVM_Value a[9];
    fvm_value_number(a, columns);
    fvm_value_number(a + 1, rows);
    a[2] = ref(kernel_array(c, columns * rows, values));
    fvm_value_number(a + 3, divisor);
    fvm_value_number(a + 4, bias);
    fvm_value_boolean(a + 5, preserve);
    fvm_value_boolean(a + 6, clamp);
    fvm_value_number(a + 7, color);
    fvm_value_number(a + 8, alpha);
    FVM_Object *effect = 0;
    if (!(fvm_construct(c, "flash.filters.ConvolutionFilter", 9, a, &effect) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, effect) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return effect;
}


static inline FVM_Object *convolution_packet(FVM_Context *c, FVM_Object *packet)
{
    FVM_Value effect, source, rect, point;
    if (!(fvm_object_get(c, packet, "effect", &effect) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, packet, "source", &source) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, packet, "rect", &rect) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, packet, "point", &point) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, packet) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value copied = call(c, effect.object, "clone");
    if (!(fvm_gc_pin(c, copied.object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(copied.object != effect.object))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *array = matrix_copy(c, copied.object);
    near(matrix_item(c, array, 0), 1);
    near(get(c, copied.object, "matrixX"), 3);
    FVM_Value a[4];
    filter_args(a, source.object, rect.object, point.object, copied.object);
    call(c, source.object, "applyFilter", 4, a);
    if (!(read(c, source.object, 0, 0) == 0x401b0000UL && read(c, source.object, 1, 0) == 0x805a0000UL &&
          read(c, source.object, 2, 0) == 0xc0990000UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return copied.object;
}


static FVM_Result
reentrant_kernel_entry(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *u)
{
    FVM_Value value;
    fvm_value_number(&value, 7);
    if (!(fvm_object_set(c, (FVM_Object *)u, "bias", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, 1);
    return FVM_OK;
}
#ifndef FVM_CONVOLUTION_FIXTURE_ONLY


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
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *empty = filter(c, 0, "flash.filters.ConvolutionFilter");
    near(get(c, empty, "matrixX"), 0);
    near(get(c, empty, "matrixY"), 0);
    near(get(c, empty, "divisor"), 1);
    near(get(c, empty, "bias"), 0);
    near(get(c, empty, "color"), 0);
    near(get(c, empty, "alpha"), 0);
    FVM_Value flag;
    if (!(fvm_object_get(c, empty, "preserveAlpha", &flag) == FVM_OK && flag.type == FVM_VALUE_BOOLEAN && flag.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    near(get(c, matrix_copy(c, empty), "length"), 0);
    double weights[] = {1, 1, 1};
    FVM_Object *effect = convolution(c, 3, 1, weights, 3), *source = bitmap(c, 3, 1, true, 0x400a0000UL),
               *dest = bitmap(c, 5, 1, true, 0);
    write(c, source, 1, 0, 0x803c0000UL);
    write(c, source, 2, 0, 0xc0c80000UL);
    double box[] = {0, 0, 3, 1};
    FVM_Object *rect = region_object(c, "flash.geom.Rectangle", 4, box), *point = hit_point(c, 0, 0);
    FVM_Value a[4];
    filter_args(a, source, rect, point, effect);
    call(c, dest, "applyFilter", 4, a);
    if (!(read(c, dest, 0, 0) == 0x401b0000UL && read(c, dest, 1, 0) == 0x805a0000UL &&
          read(c, dest, 2, 0) == 0xc0990000UL && read(c, dest, 3, 0) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *array = matrix_copy(c, effect), *second_array = matrix_copy(c, effect);
    if (!(array != second_array))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    palette_entry(c, array, 0, 99);
    near(matrix_item(c, matrix_copy(c, effect), 0), 1);
    FVM_Value clone = call(c, effect, "clone");
    if (!(fvm_gc_pin(c, clone.object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    set(c, clone.object, "bias", 10);
    near(get(c, effect, "bias"), 0);
    near(get(c, clone.object, "bias"), 10);
    set(c, effect, "matrixX", 1);
    near(get(c, matrix_copy(c, effect), "length"), 3);
    set(c, effect, "matrixY", 4);
    array = matrix_copy(c, effect);
    near(get(c, array, "length"), 4);
    near(matrix_item(c, array, 3), 0);
    set(c, effect, "matrixX", 3);
    set(c, effect, "matrixY", 1);
    FVM_Value reset = ref(kernel_array(c, 3, weights));
    if (!(fvm_object_set(c, effect, "matrix", &reset) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *replace = convolution(c, 3, 1, weights, 3, 0, true, false, 0x0000ff, 0.5);
    filter_args(a, source, rect, point, replace);
    call(c, dest, "applyFilter", 4, a);
    if (!(read(c, dest, 0, 0) == 0x40170055UL && read(c, dest, 1, 0) == 0x805a0000UL &&
          read(c, dest, 2, 0) == 0xc0570055UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_boolean(&flag, 0);
    if (!(fvm_object_set(c, replace, "preserveAlpha", &flag) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    call(c, dest, "applyFilter", 4, a);
    if (!(read(c, dest, 0, 0) == 0x6b170055UL && read(c, dest, 2, 0) == 0x95570055UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    double one[] = {1}, wide_box[] = {0, 0, 5, 1};
    FVM_Object *wide = region_object(c, "flash.geom.Rectangle", 4, wide_box),
               *identity = convolution(c, 1, 1, one, 0, 0, true, false, 0x0000ff, 0.5);
    filter_args(a, source, wide, point, identity);
    call(c, dest, "applyFilter", 4, a);
    if (!(read(c, dest, 3, 0) == 0x800000ffUL && read(c, dest, 4, 0) == 0x800000ffUL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_boolean(&flag, 1);
    if (!(fvm_object_set(c, identity, "clamp", &flag) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    call(c, dest, "applyFilter", 4, a);
    if (!(read(c, dest, 3, 0) == 0xc0c80000UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    double halo_box[] = {1, 0, 1, 1};
    FVM_Object *halo = region_object(c, "flash.geom.Rectangle", 4, halo_box);
    filter_args(a, source, halo, point, effect);
    call(c, dest, "applyFilter", 4, a);
    if (!(read(c, dest, 0, 0) == 0x805a0000UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    double even_weights[] = {1, 0};
    FVM_Object *even = convolution(c, 2, 1, even_weights);
    filter_args(a, source, rect, point, even);
    call(c, dest, "applyFilter", 4, a);
    if (!(read(c, dest, 0, 0) == 0x400a0000UL && read(c, dest, 1, 0) == 0x800a0000UL &&
          read(c, dest, 2, 0) == 0xc03c0000UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    double nine[] = {1, 1, 1, 1, 1, 1, 1, 1, 1}, two_box[] = {0, 0, 2, 2};
    FVM_Object *two = bitmap(c, 2, 2, false, 0x00010000UL),
               *two_rect = region_object(c, "flash.geom.Rectangle", 4, two_box), *blur = convolution(c, 3, 3, nine, 9);
    write(c, two, 1, 0, 0xff020000UL);
    write(c, two, 0, 1, 0xff030000UL);
    write(c, two, 1, 1, 0xff040000UL);
    filter_args(a, two, two_rect, point, blur);
    call(c, two, "applyFilter", 4, a);
    if (!(read(c, two, 0, 0) == 0xff020000UL && read(c, two, 1, 1) == 0xff030000UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *packet = 0;
    if (!(fvm_object_create(c, &packet) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, packet) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value value = ref(effect);
    if (!(fvm_object_set(c, packet, "effect", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    value = ref(source);
    if (!(fvm_object_set(c, packet, "source", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    value = ref(rect);
    if (!(fvm_object_set(c, packet, "rect", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    value = ref(point);
    if (!(fvm_object_set(c, packet, "point", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    value = ref(packet);
    FVM_Channel *ch = 0;
    FVM_Value copy;
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
    convolution_packet(other, copy.object);
    fvm_channel_destroy(ch);
    FVM_Result opened = fvm_channel_open_file("convolution-test.bin", "w", &ch);
    if (opened == FVM_OK)
    {
        if (!(fvm_channel_send_value(c, ch, &value, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_channel_destroy(ch);
        if (!(fvm_channel_open_file("convolution-test.bin", "r", &ch) == FVM_OK))
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
        convolution_packet(other, copy.object);
        if (!(std::remove("convolution-test.bin") == 0))
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
    convolution_packet(other, copy.object);
    fvm_channel_release(network);
    fvm_broker_stop(broker);
    fvm_channel_release(queue);
    fvm_broker_release(broker);
    fvm_serialized_free(uri);
#endif
    FVM_Value bounds_args[] = {ref(rect), ref(effect)};
    FVM_Value bounds = call(c, dest, "generateFilterRect", 2, bounds_args);
    if (!(bounds.object != rect))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    near(get(c, bounds.object, "width"), 3);
    unsigned long previous = rt->config.memory_limit_bytes;
    rt->config.memory_limit_bytes = 4;
    filter_args(a, source, halo, point, effect);
    FVM_Value out;
    if (!(fvm_object_call(c, dest, "applyFilter", 4, a, &out) == FVM_OUT_OF_MEMORY))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    rt->config.memory_limit_bytes = previous;
    if (!(read(c, dest, 0, 0) == 0x400a0000UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *reentrant = 0;
    if (!(fvm_object_create(c, &reentrant) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, reentrant) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    set(c, reentrant, "length", 1);
    if (!(fvm_object_define_accessor_rooted(c, reentrant, "0", reentrant_kernel_entry, 0, effect, effect) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    value = ref(reentrant);
    if (!(fvm_object_set(c, effect, "matrix", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    near(get(c, effect, "bias"), 7);
    near(matrix_item(c, matrix_copy(c, effect), 0), 1);
    set(c, effect, "matrixX", 99);
    near(get(c, effect, "matrixX"), 15);
    set(c, effect, "matrixY", -1);
    near(get(c, effect, "matrixY"), 0);
    set(c, effect, "alpha", 2);
    near(get(c, effect, "alpha"), 1);
    set(c, effect, "color", 0xffffffffUL);
    near(get(c, effect, "color"), 0xffffffUL);
    FVM_Convolution data;
    if (!(fvm_convolution_values(c, effect, data)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    value = ref(effect);
    unsigned char *bytes = 0;
    size_t size = 0;
    if (!(fvm_value_serialize(c, &value, &bytes, &size) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_serialized_free(bytes);
    effect->resource_result.object->properties["matrixX"].number = 16;
    if (!(fvm_value_serialize(c, &value, &bytes, &size) == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    effect->resource_result.object->properties["matrixX"].number = 15;
    FVM_Object *private_data = effect->resource_result.object;
    double previous_coefficient = private_data->properties["0"].number;
    private_data->properties["0"].number = 12345.75;
    if (!(fvm_value_serialize(c, &value, &bytes, &size) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    double marker = 12345.75, invalid_coefficient = 1000001.0, one_number = 1;
    unsigned char marker_bytes[8], invalid_bytes[8], one_bytes[8], pattern[8], changed[8];
    std::memcpy(marker_bytes, &marker, 8);
    std::memcpy(invalid_bytes, &invalid_coefficient, 8);
    std::memcpy(one_bytes, &one_number, 8);
    bool little = one_bytes[0] == 0;
    for (unsigned i = 0; i < 8; ++i)
    {
        pattern[i] = marker_bytes[little ? 7 - i : i];
        changed[i] = invalid_bytes[little ? 7 - i : i];
    }
    unsigned matches = 0;
    for (size_t i = 0; i + 8 <= size; ++i)
    {
        if (std::memcmp(bytes + i, pattern, 8) == 0)
        {
            std::memcpy(bytes + i, changed, 8);
            ++matches;
        }
    }
    if (!(matches == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_deserialize(other, bytes, size, &out) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_serialized_free(bytes);
    private_data->properties["0"].number = previous_coefficient;
    fvm_value_number(&value, std::numeric_limits<double>::infinity());
    if (!(fvm_object_set(c, effect, "bias", &value) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    near(get(c, effect, "bias"), 7);
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
#endif
