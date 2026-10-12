#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/movieclip.h"
#include "flashvm/transfer.h"
#include "flashvm/gc.h"
#include "flashvm/capability.h"
#include "flashvm/serialization.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static const unsigned char function_code[] = {0x96, 5, 0, 7, 42, 0, 0, 0, 0x3e, 0};
static const unsigned char frame_code[] = {0x96, 6, 0, 0,  't', 'i', 'c', 'k',  0, 0x96,
                                           5,    0, 7, 42, 0,   0,   0,   0x1d, 0};
static void draw_coordinate(FVM_Context *c, FVM_Object *clip, const char *method, double x,
                            double y)
{
    FVM_Value a[2], out;
    fvm_value_number(a, x);
    fvm_value_number(a + 1, y);
    assert(fvm_object_call(c, clip, method, 2, a, &out) == FVM_OK);
}
static void check(FVM_Context *c, FVM_Object *root)
{
    FVM_Value v, out;
    int same;
    FVM_Value ctor;
    {
        const unsigned short expected[] = {0x41, 0, 0xdfff};
        unsigned short units[3];
        size_t length = 0;
        assert(fvm_object_get(c, root, "utf16", &v) == FVM_OK);
        assert(fvm_object_call(c, v.object, "valueOf", 0, 0, &out) == FVM_OK);
        assert(fvm_value_get_utf16(&out, units, 3, &length) == FVM_OK && length == 3);
        assert(!memcmp(units, expected, sizeof(units)));
    }
    assert(fvm_object_get(c, root, "boolean", &v) == FVM_OK);
    assert(fvm_object_call(c, v.object, "valueOf", 0, 0, &out) == FVM_OK && out.boolean);
    assert(fvm_object_get(c, root, "number", &v) == FVM_OK);
    assert(fvm_object_call(c, v.object, "valueOf", 0, 0, &out) == FVM_OK && out.number == 123.5);
    assert(fvm_object_get(c, root, "string", &v) == FVM_OK);
    assert(fvm_object_get(c, v.object, "length", &out) == FVM_OK && out.number == 4);
    assert(fvm_object_get(c, root, "error", &v) == FVM_OK);
    assert(fvm_object_call(c, v.object, "toString", 0, 0, &out) == FVM_OK &&
           !strcmp(out.string, "portable"));
    assert(fvm_object_get(c, root, "date", &v) == FVM_OK);
    assert(fvm_object_call(c, v.object, "getUTCFullYear", 0, 0, &out) == FVM_OK &&
           out.number == 1970);
    assert(fvm_object_get(c, root, "array", &v) == FVM_OK);
    assert(fvm_object_get(c, v.object, "length", &out) == FVM_OK && out.number == 3);
    assert(fvm_object_get(c, v.object, "self", &out) == FVM_OK && out.object == v.object);
    assert(fvm_object_get(c, root, "function", &v) == FVM_OK);
    assert(fvm_function_invoke(c, v.object, 0, 0, 0, &out) == FVM_OK && out.number == 42);
    assert(fvm_object_get(c, root, "movie", &v) == FVM_OK);
    assert(fvm_object_get(c, v.object, "_totalframes", &out) == FVM_OK && out.number == 5);
    assert(fvm_object_get(c, v.object, "kid", &out) == FVM_OK);
    assert(fvm_object_get(c, out.object, "_parent", &out) == FVM_OK && out.object == v.object);
    assert(fvm_object_get(c, v.object, "constructor", &ctor) == FVM_OK);
    assert(fvm_value_instance_of(c, &v, &ctor, &same) == FVM_OK && same);
    assert(fvm_object_get(c, root, "alias", &out) == FVM_OK && out.object == v.object);
    {
        unsigned char pixels[32 * 128];
        FVM_RasterSurface surface;
        FVM_Result rendered;
        memset(pixels, 0, sizeof(pixels));
        surface.pixels = pixels;
        surface.width = surface.height = 32;
        surface.stride = 128;
        rendered = fvm_movieclip_render(c, v.object, &surface);
#if defined(FVM_HAS_AGG) || defined(FVM_HAS_CAIRO)
        assert(rendered == FVM_OK);
        assert(pixels[4 * 128 + 4 * 4] == 255);
        assert(pixels[4 * 128 + 4 * 4 + 3] == 255);
        assert(pixels[0] == 0 && pixels[3] == 0);
#else
        assert(rendered == FVM_UNSUPPORTED);
#endif
    }
    fvm_value_number(&out, 0);
    assert(fvm_object_set(c, v.object, "tick", &out) == FVM_OK);
    assert(fvm_movieclip_advance(c, v.object, 1) == FVM_OK);
    assert(fvm_object_get(c, v.object, "tick", &out) == FVM_OK && out.number == 42);

    assert(fvm_object_get(c, root, "self", &out) == FVM_OK && out.object == root);
    assert(fvm_object_get(c, root, "undefined", &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
    assert(fvm_object_get(c, root, "null", &out) == FVM_OK && out.type == FVM_VALUE_NULL);
    fvm_value_number(&out, 1);
    assert(fvm_object_set(c, root, "watched", &out) == FVM_OK);
    assert(fvm_object_get(c, root, "watched", &out) == FVM_OK && out.number == 42);
}
static void write_file(const char *path)
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Object *root = 0, *o = 0;
    FVM_Channel *file = 0;
    FVM_Value arg, v, out, args[2];
    const char *names[] = {"Boolean", "Number", "String", "Error", "Date", "Array"};
    const char *keys[] = {"boolean", "number", "string", "error", "date", "array"};
    unsigned int i;
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_construct(c, "Object", 0, 0, &root) == FVM_OK);
    for (i = 0; i < 6; ++i)
    {
        if (i == 0)
            fvm_value_boolean(&arg, 1);
        else if (i == 1)
            fvm_value_number(&arg, 123.5);
        else if (i == 2)
            fvm_value_string(&arg, "A\360\237\230\200Z");
        else if (i == 3)
            fvm_value_string(&arg, "portable");
        else
            fvm_value_number(&arg, i == 4 ? 0 : 3);
        assert(fvm_construct(c, names[i], 1, &arg, &o) == FVM_OK);
        fvm_value_undefined(&v);
        v.type = FVM_VALUE_OBJECT;
        v.object = o;
        assert(fvm_object_set(c, root, keys[i], &v) == FVM_OK);
        if (i == 5)
            assert(fvm_object_set(c, o, "self", &v) == FVM_OK);
    }
    {
        const unsigned short units[] = {0x41, 0, 0xdfff};
        assert(fvm_value_string_utf16(c, units, 3, &arg) == FVM_OK);
        assert(fvm_construct(c, "String", 1, &arg, &o) == FVM_OK);
        fvm_value_undefined(&v);
        v.type = FVM_VALUE_OBJECT;
        v.object = o;
        assert(fvm_object_set(c, root, "utf16", &v) == FVM_OK);
    }
    assert(fvm_function_create_avm1(c, function_code, sizeof(function_code), 0, 0, 0, &o) ==
           FVM_OK);
    v.object = o;
    assert(fvm_object_set(c, root, "function", &v) == FVM_OK);
    fvm_value_string(args, "watched");
    args[1] = v;
    assert(fvm_object_call(c, root, "watch", 2, args, &out) == FVM_OK && out.boolean);

    assert(fvm_movieclip_create(c, "root", 5, &o) == FVM_OK);
    v.object = o;
    assert(fvm_object_set(c, root, "movie", &v) == FVM_OK);
    assert(fvm_object_set(c, root, "alias", &v) == FVM_OK);
    fvm_value_string(args, "kid");
    fvm_value_number(args + 1, 7);
    assert(fvm_object_call(c, o, "createEmptyMovieClip", 2, args, &out) == FVM_OK);
    {
        FVM_Object *child = out.object;
        fvm_value_number(&arg, 16711680);
        assert(fvm_object_call(c, child, "beginFill", 1, &arg, &out) == FVM_OK);
        draw_coordinate(c, child, "moveTo", 2, 2);
        draw_coordinate(c, child, "lineTo", 6, 2);
        draw_coordinate(c, child, "lineTo", 6, 6);
        draw_coordinate(c, child, "lineTo", 2, 6);
        draw_coordinate(c, child, "lineTo", 2, 2);
        assert(fvm_object_call(c, child, "endFill", 0, 0, &out) == FVM_OK);
    }
    assert(fvm_movieclip_define_frame(c, o, 1, "one", frame_code, sizeof(frame_code)) == FVM_OK);
    assert(fvm_movieclip_define_frame(c, o, 2, "two", frame_code, sizeof(frame_code)) == FVM_OK);
    v.object = root;
    assert(fvm_object_set(c, root, "self", &v) == FVM_OK);
    fvm_value_undefined(&arg);
    assert(fvm_object_set(c, root, "undefined", &arg) == FVM_OK);
    arg.type = FVM_VALUE_NULL;
    assert(fvm_object_set(c, root, "null", &arg) == FVM_OK);
    check(c, root);
#ifdef OS_PORTS
    if (!strncmp(path, "fvm://", 6))
        assert(fvm_channel_open_network(c, path, &file) == FVM_OK);
    else
#endif
        assert(fvm_channel_open_file(path, "w", &file) == FVM_OK);
    assert(fvm_channel_send_value(c, file, &v, 0) == FVM_OK);
    fvm_channel_release(file);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
static void read_file(const char *path)
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Channel *file = 0;
    FVM_Value v;
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_channel_open_file(path, "r", &file) == FVM_OK);
    assert(fvm_channel_receive_value(c, file, &v, 0) == FVM_OK);
    assert(fvm_gc_pin(c, v.object) == FVM_OK);
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    check(c, v.object);
    fvm_channel_release(file);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
#ifdef OS_PORTS
static void network_receiver(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_ResourceBroker *broker = 0;
    FVM_Channel *queue = 0;
    FVM_Object *wrapper = 0;
    FVM_Value graph;
    char *uri = 0;
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_broker_start("127.0.0.1", "127.0.0.1", 0, &broker) == FVM_OK);
    assert(fvm_context_set_broker(c, broker) == FVM_OK);
    assert(fvm_channel_create(&queue) == FVM_OK);
    assert(fvm_extensions_wrap_channel(c, queue, &wrapper) == FVM_OK);
    assert(fvm_broker_export(c, wrapper, &uri) == FVM_OK);
    assert(puts(uri) >= 0 && !fflush(stdout));
    fvm_serialized_free(uri);
    /* A fresh process restores the complete graph without standard installation. */
    assert(fvm_channel_receive_value(c, queue, &graph, 10000) == FVM_OK);
    assert(fvm_gc_pin(c, graph.object) == FVM_OK);
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    check(c, graph.object);
    fvm_broker_stop(broker);
    fvm_broker_release(broker);
    fvm_channel_release(queue);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
#endif
int main(int argc, char **argv)
{
#ifdef OS_PORTS
    if (argc == 2 && !strcmp(argv[1], "network-receiver"))
    {
        network_receiver();
        return 0;
    }
#endif
    if (argc == 3)
    {
        if (!strcmp(argv[1], "write"))
            write_file(argv[2]);
        else
        {
            assert(!strcmp(argv[1], "read"));
            read_file(argv[2]);
        }
    }
    else
    {
        assert(argc == 1);
        write_file("objects-transfer.dat");
        read_file("objects-transfer.dat");
        assert(!remove("objects-transfer.dat"));
    }
    return 0;
}
