#define FVM_BITMAP_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "standard_bitmap.cpp"
#include "flashvm/movieclip.h"
#include "flashvm/extensions.h"
#include "flashvm/capability.h"


static FVM_Object *child(FVM_Context *c, FVM_Object *root, const char *name, double depth)
{
    FVM_Value a[2];
    fvm_value_string(a, name);
    fvm_value_number(a + 1, depth);
    return call(c, root, "createEmptyMovieClip", 2, a).object;
}


static void geometry(FVM_Context *c, FVM_Object *o, bool finish)
{
    const double xy[] = {0, 0, 8, 0, 8, 8, 0, 8, 0, 0};
    for (unsigned i = 0; i < 10; i += 2)
    {
        FVM_Value a[2];
        fvm_value_number(a, xy[i]);
        fvm_value_number(a + 1, xy[i + 1]);
        call(c, o, i ? "lineTo" : "moveTo", 2, a);
    }
    if (finish)
    {
        call(c, o, "endFill");
    }
}


static FVM_Result render(FVM_Context *c, FVM_Object *o, unsigned char *pixels)
{
    FVM_RasterSurface surface = {pixels, 8, 8, 32};
    std::memset(pixels, 0, 256);
    FVM_Result r = fvm_movieclip_render(c, o, &surface);
    if (!(r == FVM_OK || r == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return r;
}


static void verify(FVM_Context *c, FVM_Object *o, unsigned blue)
{
    unsigned char pixels[256];
    if (render(c, o, pixels) == FVM_OK)
    {
        if (!(pixels[0] == 0 && pixels[1] == 0 && pixels[2] == blue && pixels[3] == 255))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(pixels[8] == 0 && pixels[9] == 255 && pixels[10] == 0 && pixels[11] == 255))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
}


static void
begin(FVM_Context *c, FVM_Object *o, FVM_Object *image, FVM_Object *matrix, bool repeat = true, bool smooth = false)
{
    FVM_Value a[4];
    a[0] = ref(image);
    fvm_value_undefined(a + 1);
    if (matrix)
    {
        a[1] = ref(matrix);
    }
    fvm_value_boolean(a + 2, repeat);
    fvm_value_boolean(a + 3, smooth);
    call(c, o, "beginBitmapFill", 4, a);
}


static FVM_Object *property(FVM_Context *c, FVM_Object *o, const char *name)
{
    FVM_Value v;
    if (!(fvm_object_get(c, o, name, &v) == FVM_OK && v.type == FVM_VALUE_OBJECT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return v.object;
}


static void transferred(FVM_Context *c, FVM_Object *bundle)
{
    FVM_Object *clip = property(c, bundle, "clip"), *image = property(c, bundle, "image");
    FVM_Object *data = clip->resource_result.object, *paths = data->properties["paths"].object;
    if (!(paths->indexed[0].object->properties["bitmap"].object->properties["source"].object == image))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, bundle) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    verify(c, clip, 255);
    write(c, image, 0, 0, 0xff000080UL);
    verify(c, clip, 128);
}
#ifndef FVM_BITMAP_FILL_FIXTURE_ONLY


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
    FVM_Object *clip = child(c, root, "texture", 1), *image = bitmap(c, 2, 1, true, 0xff0000ffUL);
    write(c, image, 1, 0, 0xff00ff00UL);
    double m[] = {2, 0, 0, 2, 0, 0};
    FVM_Object *matrix = make(c, "flash.geom.Matrix", 6, m);
    begin(c, clip, image, matrix);
    geometry(c, clip, false);
    FVM_Value scale;
    fvm_value_number(&scale, 100);
    if (!(fvm_object_set(c, matrix, "a", &scale) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    verify(c, clip, 255);
    FVM_Object *saved_style = clip->resource_result.object->properties["bitmap"].object;
    double bad_values[] = {0, 0, 0, 1, 0, 0};
    FVM_Object *bad = make(c, "flash.geom.Matrix", 6, bad_values);
    FVM_Value bad_args[2] = {ref(image), ref(bad)}, bad_out;
    if (!(fvm_object_call(c, clip, "beginBitmapFill", 2, bad_args, &bad_out) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(clip->resource_result.object->properties["bitmap"].object == saved_style))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_unpin(c, matrix) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_unpin(c, image) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    verify(c, clip, 255);
    FVM_Value a[2];
    fvm_value_string(a, "duplicate");
    fvm_value_number(a + 1, 2);
    FVM_Object *copy = call(c, clip, "duplicateMovieClip", 2, a).object;
    if (!(copy))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *data = clip->resource_result.object, *copied = copy->resource_result.object;
    if (!(copied->properties["bitmap"].object == data->properties["bitmap"].object))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(copied->properties["active"].object != data->properties["active"].object))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(copied->properties["active"].object->properties["bitmap"].object == data->properties["bitmap"].object))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    verify(c, copy, 255);
    write(c, image, 0, 0, 0xff000080UL);
    verify(c, clip, 128);
    verify(c, copy, 128);
    write(c, image, 0, 0, 0xff0000ffUL);
    FVM_Object *bundle = 0;
    if (!(fvm_object_create(c, &bundle) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, bundle) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value v = ref(clip);
    if (!(fvm_object_set(c, bundle, "clip", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    v = ref(image);
    if (!(fvm_object_set(c, bundle, "image", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Channel *ch = 0;
    FVM_Value packet = ref(bundle), out;
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
    transferred(other, out.object);
    fvm_channel_destroy(ch);
    FVM_Result opened = fvm_channel_open_file("bitmap-fill-test.bin", "w", &ch);
    if (opened == FVM_OK)
    {
        if (!(fvm_channel_send_value(c, ch, &packet, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_channel_destroy(ch);
        if (!(fvm_channel_open_file("bitmap-fill-test.bin", "r", &ch) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_channel_receive_value(other, ch, &out, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        transferred(other, out.object);
        fvm_channel_destroy(ch);
        if (!(std::remove("bitmap-fill-test.bin") == 0))
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
    if (!(fvm_channel_send_value(c, queue, &packet, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_receive_value(other, network, &out, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    transferred(other, out.object);
    fvm_channel_release(network);
    fvm_broker_stop(broker);
    fvm_channel_release(queue);
    fvm_broker_release(broker);
    fvm_serialized_free(uri);
#endif
    call(c, clip, "endFill");
    if (!(data->properties.find("bitmap") == data->properties.end()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    verify(c, clip, 255);
    FVM_Value solid;
    fvm_value_number(&solid, 0xff0000);
    call(c, clip, "beginFill", 1, &solid);
    if (!(data->properties.find("bitmap") == data->properties.end()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    call(c, clip, "clear");
    unsigned char pixels[256];
    if (render(c, clip, pixels) == FVM_OK)
    {
        for (unsigned i = 0; i < 256; ++i)
        {
            if (!(pixels[i] == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    verify(c, copy, 255);
    call(c, image, "dispose");
    if (render(c, copy, pixels) == FVM_OK)
    {
        for (unsigned i = 0; i < 256; ++i)
        {
            if (!(pixels[i] == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
#endif
