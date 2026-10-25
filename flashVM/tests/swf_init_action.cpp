/* DoInitAction ordering, one-shot execution, exceptions, reentry and transfer. */
#define main sprite_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "swf_sprite_resources.cpp"

#undef main
#include "flashvm/function.h"
#include "../src/core/runtime.hpp"


static void initializer(Bytes &tags, const Bytes &code, unsigned id = 7)
{
    Bytes data;
    u16(data, id);
    for (size_t i = 0; i < code.size(); ++i)
    {
        data.push_back(code[i]);
    }
    tag(tags, 59, data);
}


static Bytes observe()
{
    Bytes b;
    variable(b, "this");
    text(b, "observed");
    variable(b, "this");
    text(b, "seen");
    b.push_back(0x4e);
    b.push_back(0x4f);
    b.push_back(0);
    return b;
}


static FVM_Result reenter(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    FVM_Object *root = (FVM_Object *)raw;
    if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 6; version <= 8; ++version)
    {
        FVM_Context *c = 0, *other = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes empty, controls, tags;
        tag(controls, 0, empty);
        tag(tags, 39, sprite(7, 0, controls));
        for (unsigned i = 1; i <= 20; ++i)
        {
            initializer(tags, action(i));
        }
        tag(tags, 12, observe());
        tag(tags, 1, empty);
        tag(tags, 0, empty);
        Bytes swf = movie(tags, 1);
        swf[3] = (unsigned char)version;
        FVM_Object *root = 0;
        if (!(fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "root", &root) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(c, root) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(get(c, root, "seen").type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, root, "observed", 20);
        number(c, root, "seen", 20);
        FVM_Value v;
        fvm_value_number(&v, 91);
        if (!(fvm_object_set(c, root, "seen", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_advance(c, root, 3) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, root, "seen", 91);
        FVM_Channel *channel = 0;
        if (!(fvm_channel_create(&channel) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&v);
        v.type = FVM_VALUE_OBJECT;
        v.object = root;
        FVM_Value out;
        if (!(fvm_channel_send_value(c, channel, &v, 0) == FVM_OK &&
              fvm_channel_receive_value(other, channel, &out, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *copy = out.object;
        if (!(fvm_gc_pin(other, copy) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_advance(other, copy, 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(other, copy, "seen", 20);
        if (!(fvm_movieclip_advance(other, copy, 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_channel_destroy(channel);
        Bytes throwing;
        text(throwing, "init failure");
        throwing.push_back(0x2a);
        throwing.push_back(0);
        tags.clear();
        tag(tags, 39, sprite(7, 0, controls));
        initializer(tags, throwing);
        initializer(tags, action(44));
        tag(tags, 12, observe());
        tag(tags, 1, empty);
        tag(tags, 0, empty);
        swf = movie(tags, 1);
        FVM_Object *failed = 0;
        if (!(fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "failed", &failed) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(c, failed) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_advance(c, failed, 1) == FVM_EXCEPTION))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exception_clear(c) == FVM_OK && !c->script_this && !c->gc_frames))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_advance(c, failed, 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, failed, "observed", 44);
        Bytes callback;
        integer(callback, 0);
        text(callback, "reenterInit");
        callback.push_back(0x3d);
        callback.push_back(0x17);
        callback.push_back(0);
        tags.clear();
        tag(tags, 39, sprite(7, 0, controls));
        initializer(tags, callback);
        initializer(tags, action(52));
        tag(tags, 1, empty);
        tag(tags, 0, empty);
        swf = movie(tags, 1);
        FVM_Object *recursive = 0, *native = 0;
        if (!(fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "recursive", &recursive) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_create_native(c, reenter, recursive, recursive, &native) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        v.object = native;
        if (!(fvm_set_global(c, "reenterInit", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_advance(c, recursive, 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, recursive, "seen", 52);
        Bytes normal;
        tag(normal, 1, empty);
        tag(normal, 0, empty);
        tags.clear();
        tag(tags, 39, sprite(7, 1, normal));
        export_asset(tags, 7, "Hero");
        initializer(tags, action(73));
        tag(tags, 12, observe());
        tag(tags, 1, empty);
        tag(tags, 0, empty);
        swf = movie(tags, 1);
        FVM_Object *source = 0;
        if (!(fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "source", &source) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(c, source) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *child = attach(c, source, "Hero", "child", 1);
        if (!(child))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_advance(c, child, 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, source, "seen", 73);
        if (!(get(c, child, "seen").type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_advance(c, source, 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, source, "observed", 73);
        Bytes bad;
        tag(bad, 39, sprite(7, 0, controls));
        initializer(bad, action(1), 8);
        tag(bad, 1, empty);
        tag(bad, 0, empty);
        rejects(c, bad, FVM_INVALID_ARGUMENT);
        bad.clear();
        tag(bad, 39, sprite(7, 0, controls));
        Bytes short_data;
        short_data.push_back(7);
        tag(bad, 59, short_data);
        tag(bad, 1, empty);
        tag(bad, 0, empty);
        rejects(c, bad, FVM_INVALID_ARGUMENT);
        fvm_context_destroy(other);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
