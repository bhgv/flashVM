#define FVM_LOSSLESS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "swf_lossless_bitmap.cpp"
#include "flashvm/extensions.h"
#include "flashvm/capability.h"


static inline Bytes bitmap_shape(unsigned kind = 0x42, unsigned bitmap_id = 7)
{
    Bytes b;
    u16(b, 9);
    Bits bounds;
    bounds.put(16, 5);
    bounds.put(0, 16);
    bounds.put(320, 16);
    bounds.put(0, 16);
    bounds.put(320, 16);
    append(b, bounds.b);
    b.push_back(1);
    b.push_back((unsigned char)kind);
    u16(b, bitmap_id);
    Bits m;
    m.put(1, 1);
    m.put(22, 5);
    m.put(20 * 65536, 22);
    m.put(20 * 65536, 22);
    m.put(0, 1);
    m.put(0, 5);
    append(b, m.b);
    b.push_back(0);
    Bits records;
    records.put(1, 4);
    records.put(0, 4);
    records.put(0, 1);
    records.put(3, 5);
    records.put(16, 5);
    records.put(0, 16);
    records.put(0, 16);
    records.put(1, 1);
    edge(records, 320, 0);
    edge(records, 0, 320);
    edge(records, -320, 0);
    edge(records, 0, -320);
    records.put(0, 6);
    append(b, records.b);
    return b;
}


static inline Bytes bitmap_tags(unsigned kind = 0x42)
{
    Bytes tags, empty;
    tag(tags, 20, lossless(5, 2, 1, bitmap_raw()));
    tag(tags, 32, bitmap_shape(kind));
    tag(tags, 26, place(2 | 4, 1, 9, 0, 0));
    tag(tags, 1, empty);
    tag(tags, 0, empty);
    return tags;
}


static inline void bitmap_pixels(FVM_Context *c, FVM_Object *root, unsigned kind)
{
    unsigned char pixels[16 * 16 * 4];
    std::memset(pixels, 0, sizeof(pixels));
    FVM_RasterSurface surface = {pixels, 16, 16, 64};
    FVM_Result r = fvm_movieclip_render(c, root, &surface);
    if (r == FVM_UNSUPPORTED)
    {
        return;
    }
    if (!(r == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(pixels[0] == 255 && pixels[1] == 0 && pixels[3] == 255))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(pixels[4] == 0 && pixels[5] == 255 && pixels[7] == 255))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(pixels[8] == ((kind == 0x40 || kind == 0x42) ? 255 : 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(pixels[9] == ((kind == 0x40 || kind == 0x42) ? 0 : 255)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}
#ifndef FVM_BITMAP_SWF_FIXTURE_ONLY


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
    Bytes direct_tags, direct_empty;
    tag(direct_tags, 20, lossless(5, 2, 1, bitmap_raw()));
    tag(direct_tags, 26, place(2 | 4, 1, 7, 0, 0));
    tag(direct_tags, 1, direct_empty);
    tag(direct_tags, 0, direct_empty);
    FVM_Object *direct = load(c, direct_tags, 1);
    tick(c, direct);
    FVM_Value direct_value, direct_copy;
    fvm_value_undefined(&direct_value);
    direct_value.type = FVM_VALUE_OBJECT;
    direct_value.object = direct;
    unsigned char *direct_bytes = 0;
    size_t direct_count = 0;
    if (!(fvm_value_serialize(c, &direct_value, &direct_bytes, &direct_count) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_deserialize(other, direct_bytes, direct_count, &direct_copy) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_serialized_free(direct_bytes);
    FVM_Object *basic = direct_copy.object;
    if (!(fvm_gc_pin(other, basic) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(other, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned char direct_pixels[16 * 16 * 4];
    std::memset(direct_pixels, 0, sizeof(direct_pixels));
    FVM_RasterSurface direct_surface = {direct_pixels, 16, 16, 64};
    FVM_Result direct_render = fvm_movieclip_render(other, basic, &direct_surface);
    if (direct_render == FVM_OK)
    {
        if (!(direct_pixels[0] == 255 && direct_pixels[5] == 255))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(direct_pixels[11] == 0 && direct_pixels[64 + 3] == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(direct_render == FVM_UNSUPPORTED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    for (unsigned kind = 0x40; kind <= 0x43; ++kind)
    {
        FVM_SWFShape decoded;
        Bytes shape_data = bitmap_shape(kind);
        if (!(fvm_swf_read_shape(shape_data.begin(), shape_data.size(), 32, decoded) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(decoded.paints[0].bitmap_matrix.xx == 1 && decoded.paints[0].bitmap_matrix.yy == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *root = load(c, bitmap_tags(kind), 1);
        tick(c, root);
        bitmap_pixels(c, root, kind);
        FVM_Object *defs = root->resource_result.object->properties["library"].object->properties["sprites"].object;
        FVM_Object *data = defs->indexed[7].object->properties["bitmap_data"].object;
        FVM_Object *paint = defs->indexed[9].object->properties["paths"].object->indexed[0].object;
        if (!(paint->properties["bitmap"].object->properties["source"].object == data))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        bitmap_pixels(c, root, kind);
        FVM_Value value, copy;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = root;
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
        if (!(fvm_channel_receive_value(other, ch, &copy, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(other, copy.object) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(other, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        bitmap_pixels(other, copy.object, kind);
        fvm_channel_destroy(ch);
        unsigned char *bytes = 0;
        size_t count = 0;
        if (!(fvm_value_serialize(c, &value, &bytes, &count) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_value_deserialize(other, bytes, count, &copy) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_serialized_free(bytes);
        if (!(fvm_gc_pin(other, copy.object) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(other, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        bitmap_pixels(other, copy.object, kind);
        if (kind == 0x43)
        {
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
            if (!(fvm_gc_pin(other, copy.object) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(other, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            bitmap_pixels(other, copy.object, kind);
            fvm_channel_release(network);
            fvm_broker_stop(broker);
            fvm_channel_release(queue);
            fvm_broker_release(broker);
            fvm_serialized_free(uri);
#endif
            FVM_Result opened = fvm_channel_open_file("flashvm-swf-bitmap.bin", "w", &ch);
            if (opened == FVM_OK)
            {
                if (!(fvm_channel_send_value(c, ch, &value, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_channel_destroy(ch);
                if (!(fvm_channel_open_file("flashvm-swf-bitmap.bin", "r", &ch) == FVM_OK))
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
                if (!(std::remove("flashvm-swf-bitmap.bin") == 0))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_gc_pin(other, copy.object) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_gc_collect(other, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                bitmap_pixels(other, copy.object, kind);
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
    }
    Bytes swf = movie(bitmap_tags(), 1);
    swf[3] = 7;
    FVM_Object *invalid = 0;
    if (!(fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "bad", &invalid) == FVM_INVALID_ARGUMENT && !invalid))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes bad, empty;
    tag(bad, 32, bitmap_shape());
    tag(bad, 1, empty);
    tag(bad, 0, empty);
    swf = movie(bad, 1);
    if (!(fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "bad", &invalid) == FVM_INVALID_ARGUMENT && !invalid))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bad.clear();
    tag(bad, 20, lossless(5, 2, 1, bitmap_raw()));
    tag(bad, 20, lossless(5, 2, 1, bitmap_raw()));
    tag(bad, 1, empty);
    tag(bad, 0, empty);
    swf = movie(bad, 1);
    if (!(fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "bad", &invalid) == FVM_INVALID_ARGUMENT && !invalid))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bad.clear();
    Bytes nested;
    u16(nested, 10);
    u16(nested, 1);
    tag(nested, 20, lossless(5, 2, 1, bitmap_raw()));
    tag(nested, 1, empty);
    tag(nested, 0, empty);
    tag(bad, 39, nested);
    tag(bad, 1, empty);
    tag(bad, 0, empty);
    swf = movie(bad, 1);
    if (!(fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "bad", &invalid) == FVM_INVALID_ARGUMENT && !invalid))
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
