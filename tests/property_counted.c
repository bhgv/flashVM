/* Counted property names survive script dispatch, enumeration, GC and transport. */
#include "flashvm/standard.h"
#include "flashvm/property.h"
#include "flashvm/table.h"
#include "flashvm/function.h"
#include "flashvm/transfer.h"
#include "flashvm/serialization.h"
#include "flashvm/gc.h"
#include "flashvm/capability.h"
#include "flashvm/avm1.h"
#include "zlib.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static const char key[] = {'x', 0, 'y'};
static int watches;
static FVM_Result watched(FVM_Context *c, FVM_Object *self, unsigned int n,
                          const FVM_Value *args, FVM_Value *out, void *raw)
{
    unsigned long freed;
    (void)raw;
    assert(n == 4 && self && fvm_value_string_size(args) == sizeof(key));
    assert(!memcmp(args[0].string, key, sizeof(key)));
    assert(fvm_gc_collect(c, &freed) == FVM_INVALID_ARGUMENT);
    ++watches;
    *out = args[2];
    return FVM_OK;
}
static void check(FVM_Context *c, FVM_Object *object)
{
    FVM_Value out;
    int own;
    assert(fvm_object_get_bytes(c, object, key, sizeof(key), &out) == FVM_OK && out.number == 42);
    assert(fvm_object_get(c, object, "x", &out) == FVM_OK && out.number == 7);
    assert(fvm_object_has_own_bytes(c, object, key, sizeof(key), &own) == FVM_OK && own);
}
int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    FVM_Context *c, *other;
    FVM_Object *object, *prototype, *keys, *fn, *channel_object;
    FVM_Value value, out, args[3], graph;
    FVM_Channel *queue, *file;
    unsigned char *data;
    size_t size, i, count;
    unsigned int flags;
    unsigned long freed;
    int own, deleted, found;
    const char scalar[] = "\xf0\x90\x80\x80";
    const char pair[] = "\xed\xa0\x80\xed\xb0\x80";
    const char lone[] = "\xed\xbf\xbf";
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_context_create(rt, &other) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK);
    assert(fvm_gc_pin(c, object) == FVM_OK);
    fvm_value_number(&value, 7);
    assert(fvm_object_set(c, object, "x", &value) == FVM_OK);
    fvm_value_number(&value, 42);
    assert(fvm_object_set_bytes(c, object, key, sizeof(key), &value) == FVM_OK);
    check(c, object);
    assert(fvm_object_set_bytes(c, object, pair, sizeof(pair)-1, &value) == FVM_OK);
    assert(fvm_object_get(c, object, scalar, &out) == FVM_OK && out.number == 42);
    assert(fvm_object_set(c, object, lone, &value) == FVM_OK);
    assert(fvm_object_get(c, object, lone, &out) == FVM_OK && out.number == 42);
    assert(fvm_object_set_bytes(c, object, "\xff", 1, &value) == FVM_INVALID_ARGUMENT);
    assert(fvm_table_named_count(c, object, &count) == FVM_OK && count == 4);
    assert(fvm_construct(c, "Object", 0, 0, &prototype) == FVM_OK);
    assert(fvm_object_set_bytes(c, prototype, "a\0b", 3, &value) == FVM_OK);
    assert(fvm_object_set_prototype(c, object, prototype) == FVM_OK);
    assert(fvm_object_get_bytes(c, object, "a\0b", 3, &out) == FVM_OK && out.number == 42);
    assert(fvm_object_property_flags_bytes(c, object, key, sizeof(key), &flags) == FVM_OK && !flags);
    assert(fvm_object_set_property_flags_bytes(c, object, key, sizeof(key), 7, 0) == FVM_OK);
    fvm_value_number(&value, 99);
    assert(fvm_object_set_bytes(c, object, key, sizeof(key), &value) == FVM_OK);
    assert(fvm_object_delete_bytes(c, object, key, sizeof(key), &deleted) == FVM_OK && !deleted);
    check(c, object);
    assert(fvm_object_set_property_flags_bytes(c, object, key, sizeof(key), 0, 7) == FVM_OK);
    assert(fvm_value_string_bytes(args, key, sizeof(key)) == FVM_OK);
    assert(fvm_object_call(c, object, "hasOwnProperty", 1, args, &out) == FVM_OK && out.boolean);
    assert(fvm_object_call(c, object, "isPropertyEnumerable", 1, args, &out) == FVM_OK && out.boolean);
    assert(fvm_object_enumerate(c, object, 0, &keys) == FVM_OK);
    found = 0;
    assert(fvm_table_indexed_count(c, keys, &count) == FVM_OK);
    for (i = 0; i < count; ++i)
    {
        assert(fvm_table_get_index(c, keys, (unsigned long)i, &out) == FVM_OK);
        if (fvm_value_string_size(&out) == sizeof(key) && !memcmp(out.string, key, sizeof(key)))
            found = 1;
    }
    assert(found);
    assert(fvm_function_create_native(c, watched, 0, 0, &fn) == FVM_OK);
    fvm_value_undefined(args + 1);args[1].type = FVM_VALUE_OBJECT;args[1].object = fn;
    assert(fvm_object_call(c, object, "watch", 2, args, &out) == FVM_OK && out.boolean);
    fvm_value_number(&value, 42);
    assert(fvm_object_set_bytes(c, object, key, sizeof(key), &value) == FVM_OK && watches == 1);
    assert(fvm_object_call(c, object, "unwatch", 1, args, &out) == FVM_OK && out.boolean);
    /* Script member lookup obtains the counted key through a global value. */
    fvm_value_undefined(&graph);graph.type = FVM_VALUE_OBJECT;graph.object = object;
    assert(fvm_set_global(c, "obj", &graph) == FVM_OK);
    assert(fvm_set_global(c, "key", args) == FVM_OK);
    {
        const unsigned char code[] = {0x96, 8,0,0,'r','e','s','u','l','t',0,
            0x96,5,0,0,'o','b','j',0,0x1c,0x96,5,0,0,'k','e','y',0,0x1c,0x4e,0x1d,0};
        assert(fvm_avm1_execute(c, code, sizeof(code)) == FVM_OK);
        assert(fvm_get_global(c, "result", &out) == FVM_OK && out.number == 42);
    }
    assert(fvm_value_serialize(c, &graph, &data, &size) == FVM_OK && data[7] == 8);
    assert(fvm_value_deserialize(other, data, size, &out) == FVM_OK);check(other, out.object);
    /* Previous graph versions must reject embedded NUL in a property identifier. */
    data[7] = 7;
    assert(fvm_value_deserialize(other, data, size, &out) == FVM_INVALID_ARGUMENT);
    data[7] = 8;
    fvm_serialized_free(data);
    /* Malformed UTF-8 and duplicate counted keys reject the graph before allocation. */
    {
        FVM_Object *plain;
        FVM_Value packet;
        unsigned long before, after, crc;
        size_t match = 0;
        const unsigned char marker[] = {0, 0, 0, 3, 'z', 0, 'b'};
        assert(fvm_object_create(c, &plain) == FVM_OK);
        fvm_value_number(&value, 42);
        assert(fvm_object_set_bytes(c, plain, "z\0a", 3, &value) == FVM_OK);
        assert(fvm_object_set_bytes(c, plain, "z\0b", 3, &value) == FVM_OK);
        fvm_value_undefined(&packet);packet.type = FVM_VALUE_OBJECT;packet.object = plain;
        assert(fvm_value_serialize(c, &packet, &data, &size) == FVM_OK);
        for (i = 16; i + sizeof(marker) <= size; ++i)
            if (!memcmp(data + i, marker, sizeof(marker)))
            { assert(!match);match = i; }
        assert(match);
        assert(fvm_gc_object_count(other, &before) == FVM_OK);
        data[match + 6] = 'a';
        crc = (unsigned long)crc32(0, data + 16, (uInt)(size - 16));
        for (i = 0; i < 4; ++i)data[12+i] = (unsigned char)(crc >> (24-i*8));
        assert(fvm_value_deserialize(other, data, size, &out) == FVM_INVALID_ARGUMENT);
        assert(fvm_gc_object_count(other, &after) == FVM_OK && before == after);
        data[match + 6] = 'b';data[match + 4] = 0xff;
        crc = (unsigned long)crc32(0, data + 16, (uInt)(size - 16));
        for (i = 0; i < 4; ++i)data[12+i] = (unsigned char)(crc >> (24-i*8));
        assert(fvm_value_deserialize(other, data, size, &out) == FVM_INVALID_ARGUMENT);
        assert(fvm_gc_object_count(other, &after) == FVM_OK && before == after);
        fvm_serialized_free(data);
    }
    assert(fvm_channel_create(&queue) == FVM_OK);
    assert(fvm_channel_send_value(c, queue, &graph, 0) == FVM_OK);
    assert(fvm_channel_receive_value(other, queue, &out, 0) == FVM_OK);check(other, out.object);
    remove("counted-keys.bin");
    assert(fvm_channel_open_file("counted-keys.bin", "w", &file) == FVM_OK);
    assert(fvm_channel_send_value(c, file, &graph, 0) == FVM_OK);
    fvm_channel_release(file);
    assert(fvm_channel_open_file("counted-keys.bin", "r", &file) == FVM_OK);
    assert(fvm_channel_receive_value(other, file, &out, 0) == FVM_OK);check(other, out.object);
    fvm_channel_release(file);assert(!remove("counted-keys.bin"));
#ifdef OS_PORTS
    {
        FVM_ResourceBroker *broker;
        FVM_Channel *network;
        char *uri;
        assert(fvm_broker_start("127.0.0.1", "127.0.0.1", 0, &broker) == FVM_OK);
        assert(fvm_context_set_broker(c, broker) == FVM_OK);
        assert(fvm_extensions_wrap_channel(c, queue, &channel_object) == FVM_OK);
        assert(fvm_broker_export(c, channel_object, &uri) == FVM_OK);
        assert(fvm_channel_open_network(other, uri, &network) == FVM_OK);
        assert(fvm_channel_send_value(c, queue, &graph, 0) == FVM_OK);
        assert(fvm_channel_receive_value(other, network, &out, 0) == FVM_OK);check(other, out.object);
        fvm_channel_release(network);fvm_broker_stop(broker);fvm_broker_release(broker);
        fvm_serialized_free(uri);
    }
#else
    (void)channel_object;
#endif
    assert(fvm_gc_collect(c, &freed) == FVM_OK);check(c, object);
    assert(fvm_object_delete_bytes(c, object, key, sizeof(key), &deleted) == FVM_OK && deleted);
    assert(fvm_object_has_own_bytes(c, object, key, sizeof(key), &own) == FVM_OK && !own);
    assert(fvm_object_has_own(c, object, "x", &own) == FVM_OK && own);
    fvm_channel_release(queue);assert(fvm_gc_unpin(c, object) == FVM_OK);
    fvm_context_destroy(other);fvm_context_destroy(c);fvm_runtime_destroy(rt);
    return 0;
}
