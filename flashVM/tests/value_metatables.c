#include <stdlib.h>
#include "flashvm/metamethod.h"
#include "flashvm/table.h"
#include "flashvm/gc.h"
#include "flashvm/gc_roots.h"
#include "flashvm/avm1.h"
#include "flashvm/serialization.h"
#include "flashvm/transfer.h"
#include "flashvm/extensions.h"
#include <string.h>
#include <stdio.h>


static FVM_Result add(FVM_Context *c, FVM_Object *self, unsigned int n, const FVM_Value *args, FVM_Value *out, void *ud)
{
    (void)c;
    (void)ud;
    if (!(!self && n == 4 && args[0].type == FVM_VALUE_NUMBER))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(args[1].type == FVM_VALUE_OBJECT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, args[2].number + args[3].number + 100);
    return FVM_OK;
}


static FVM_Result get(FVM_Context *c, FVM_Object *self, unsigned int n, const FVM_Value *args, FVM_Value *out, void *ud)
{
    (void)c;
    (void)ud;
    (void)self;
    if (!(n == 3 && args[2].type == FVM_VALUE_STRING))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, (double)strlen(args[2].string));
    return FVM_OK;
}


static FVM_Result set(FVM_Context *c, FVM_Object *self, unsigned int n, const FVM_Value *args, FVM_Value *out, void *ud)
{
    (void)c;
    (void)ud;
    (void)self;
    if (!(n == 4 && args[2].type == FVM_VALUE_STRING && args[3].number == 7))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


int main(void)
{
    FVM_Config config;
    FVM_Runtime *runtime;
    FVM_Context *c, *dst;
    FVM_Object *meta, *foreign, *found;
    FVM_Value value, copied, out, args[2], ns;
    FVM_Channel *channel;
    FVM_GCRootScope *roots;
    unsigned long freed;
    unsigned int i;
    /* result = decorated + 2; uses a C-created primitive global as AVM input. */
    const unsigned char actions[] = {0x96, 8, 0,   0,   'r', 'e', 's', 'u', 'l',  't',  0,   0x96, 11,
                                     0,    0, 'd', 'e', 'c', 'o', 'r', 'a', 't',  'e',  'd', 0,    0x1c,
                                     0x96, 5, 0,   7,   2,   0,   0,   0,   0x0a, 0x1d, 0};
    if (!(fvm_config_init(&config) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&config, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(runtime, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(runtime, &dst) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_extensions_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_create(c, &meta) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_create(dst, &foreign) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_meta_register(c, meta, "__add", add, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_meta_register(c, meta, "__index", get, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_meta_register(c, meta, "__newindex", set, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_serialization_register_callback(c, "test.add", add) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_serialization_register_callback(dst, "test.add", add) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_serialization_register_callback(c, "test.get", get) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_serialization_register_callback(dst, "test.get", get) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_serialization_register_callback(c, "test.set", set) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_serialization_register_callback(dst, "test.set", set) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 5);
    if (!(fvm_value_set_metatable(c, &value, meta) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    copied = value;
    if (!(fvm_value_set_metatable(c, &value, 0) == FVM_OK && !value.metatable))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(copied.metatable == meta))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_set_metatable(c, &copied, foreign) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_set_global(c, "decorated", &copied) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_set_global(dst, "bad", &copied) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, &freed) == FVM_OK && !freed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_avm1_execute(c, actions, sizeof(actions)) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_get_global(c, "result", &out) == FVM_OK && out.number == 107))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_get_property(c, &copied, "size", &out) == FVM_OK && out.number == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&out, 7);
    if (!(fvm_value_set_property(c, &copied, "property", &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_get_global(c, "metatable", &ns) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args, 9);
    fvm_value_undefined(args + 1);
    args[1].type = FVM_VALUE_OBJECT;
    args[1].object = meta;
    if (!(fvm_object_call(c, ns.object, "attach", 2, args, &out) == FVM_OK && out.number == 9))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, ns.object, "get", 1, &out, &value) == FVM_OK && value.object == meta))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_create(&channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_send(channel, &copied) == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_send_value(c, channel, &copied, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_try_receive(channel, &value) == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_receive_value(dst, channel, &value, 0) == FVM_OK && value.number == 5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_get_metatable(dst, &value, &found) == FVM_OK && found != meta))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_roots_begin(dst, &roots) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_roots_add(roots, &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(dst, &freed) == FVM_OK && freed == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    } /* foreign only */
    if (!(fvm_value_get_property(dst, &value, "abc", &out) == FVM_OK && out.number == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_roots_end(roots) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Every primitive tag can carry metadata, including undefined and null. */
    for (i = FVM_VALUE_UNDEFINED; i < FVM_VALUE_OBJECT; ++i)
    {
        fvm_value_undefined(&value);
        value.type = (FVM_ValueType)i;
        if (i == FVM_VALUE_STRING)
        {
            value.string = "text";
        }
        if (!(fvm_value_set_metatable(c, &value, meta) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_value_get_metatable(c, &value, &found) == FVM_OK && found == meta))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_channel_send_value(c, channel, &value, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_channel_receive_value(dst, channel, &out, 0) == FVM_OK && out.type == value.type))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(out.metatable))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_channel_destroy(channel);
    if (!(fvm_channel_open_file("primitive-meta.bin", "rw", &channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_send_value(c, channel, &copied, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_receive_value(dst, channel, &out, 0) == FVM_OK && out.number == 5 && out.metatable))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_get_property(dst, &out, "file", &value) == FVM_OK && value.number == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_channel_destroy(channel);
    if (!(!remove("primitive-meta.bin")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(dst);
    fvm_context_destroy(c);
    fvm_runtime_destroy(runtime);
    return 0;
}
