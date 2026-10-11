#include "flashvm/transfer.h"
#include "flashvm/serialization.h"
#include "flashvm/standard.h"
#include "flashvm/extensions.h"
#include "flashvm/metamethod.h"
#include "flashvm/table.h"
#include "flashvm/gc.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static FVM_Result captured(FVM_Context *c, FVM_Object *self, unsigned int count,
                           const FVM_Value *args, FVM_Value *out, void *userdata)
{
    (void)self;
    (void)count;
    (void)args;
    return fvm_object_get(c, (FVM_Object *)userdata, "answer", out);
}
static void object_value(FVM_Value *value, FVM_Object *object)
{
    fvm_value_undefined(value);
    value->type = FVM_VALUE_OBJECT;
    value->object = object;
}
int main(void)
{
    const char *path = "file-channel-test.bin";
    const char *raw_path = "file-channel-raw.bin";
    FVM_Config config;
    FVM_Runtime *runtime;
    FVM_Context *src, *dst;
    FVM_Channel *channel;
    FVM_Object *root, *child, *meta, *capture, *boxed, *restored_meta;
    FVM_Value value, copy, edge, other, args[3], namespace_value;
    unsigned char *data;
    size_t size, count, pending;
    unsigned long freed;
    int ready;
    char raw[8];
    FILE *stream;
    remove(path);
    remove(raw_path);
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &src) == FVM_OK);
    assert(fvm_context_create(runtime, &dst) == FVM_OK);
    assert(fvm_standard_install(src) == FVM_OK);
    assert(fvm_extensions_install(src) == FVM_OK);
    assert(fvm_serialization_register_callback(src, "test.capture", captured) == FVM_OK);
    assert(fvm_serialization_register_callback(dst, "test.capture", captured) == FVM_OK);
    assert(fvm_object_create(src, &root) == FVM_OK);
    assert(fvm_object_create(src, &child) == FVM_OK);
    assert(fvm_object_create(src, &meta) == FVM_OK);
    assert(fvm_object_create(src, &capture) == FVM_OK);
    fvm_value_number(&value, 42);
    assert(fvm_object_set(src, capture, "answer", &value) == FVM_OK);
    assert(fvm_meta_register_rooted(src, meta, "__index", captured, capture, capture) == FVM_OK);
    assert(fvm_table_set_metatable(src, root, meta) == FVM_OK);
    object_value(&value, child);
    assert(fvm_object_set(src, root, "child", &value) == FVM_OK);
    assert(fvm_table_set_index(src, root, 9, &value) == FVM_OK);
    object_value(&value, root);
    assert(fvm_object_set(src, child, "parent", &value) == FVM_OK);
    fvm_value_boolean(&value, 0);
    assert(fvm_construct(src, "Boolean", 1, &value, &boxed) == FVM_OK);
    object_value(&value, boxed);
    assert(fvm_object_set(src, root, "boxed", &value) == FVM_OK);
    object_value(&value, root);
    assert(fvm_value_serialize(src, &value, &data, &size) == FVM_OK);
    assert(size >= 16 && !memcmp(data, "FVMG", 4));
    assert(fvm_value_deserialize(dst, data, size - 1, &copy) == FVM_INVALID_ARGUMENT);
    data[size - 1] ^= 1;
    assert(fvm_value_deserialize(dst, data, size, &copy) == FVM_INVALID_ARGUMENT);
    data[size - 1] ^= 1;
    fvm_serialized_free(data);
    assert(fvm_channel_open_file(path, "bad", &channel) == FVM_INVALID_ARGUMENT && !channel);
    assert(fvm_channel_open_file(path, "r", &channel) == FVM_ERROR && !channel);
    /* ActionScript namespace accepts the file descriptor arguments. */
    assert(fvm_get_global(src, "channel", &namespace_value) == FVM_OK);
    assert(fvm_value_string(args, "file") == FVM_OK);
    assert(fvm_value_string(args + 1, path) == FVM_OK);
    assert(fvm_value_string(args + 2, "w") == FVM_OK);
    assert(fvm_object_call(src, namespace_value.object, "create", 3, args, &copy) == FVM_OK);
    assert(fvm_object_call(src, copy.object, "send", 1, &value, &other) == FVM_OK);
    assert(fvm_object_call(src, copy.object, "receive", 0, 0, &other) == FVM_ERROR);
    assert(fvm_object_call(src, copy.object, "close", 0, 0, &other) == FVM_OK);
    fvm_context_destroy(src); /* File has no pointers into this destroyed context. */
    assert(fvm_channel_open_file(path, "r", &channel) == FVM_OK);
    assert(fvm_channel_try_send(channel, &value) == FVM_INVALID_ARGUMENT);
    fvm_value_number(&value, 7);
    assert(fvm_channel_send(channel, &value) == FVM_ERROR);
    assert(fvm_channel_available(channel, &ready) == FVM_OK && ready);
    assert(fvm_channel_try_receive(channel, &copy) == FVM_UNSUPPORTED);
    assert(fvm_channel_receive_value(dst, channel, &copy, 1) == FVM_OK);
    assert(fvm_gc_pin(dst, copy.object) == FVM_OK);
    assert(fvm_object_get(dst, copy.object, "child", &edge) == FVM_OK);
    assert(fvm_table_get_index(dst, copy.object, 9, &other) == FVM_OK &&
           edge.object == other.object);
    assert(fvm_object_get(dst, edge.object, "parent", &other) == FVM_OK &&
           other.object == copy.object);
    assert(fvm_object_get(dst, copy.object, "missing", &other) == FVM_OK && other.number == 42);
    assert(fvm_table_get_metatable(dst, copy.object, &restored_meta) == FVM_OK && restored_meta);
    assert(fvm_object_get(dst, copy.object, "boxed", &other) == FVM_OK);
    assert(fvm_object_call(dst, other.object, "valueOf", 0, 0, &other) == FVM_OK && !other.boolean);
    assert(fvm_gc_collect(dst, &freed) == FVM_OK && !freed);
    assert(fvm_channel_try_receive(channel, &other) == FVM_NOT_FOUND);
    assert(fvm_channel_receive_value(dst, channel, &other, 1) == FVM_ERROR);
    assert(fvm_channel_set_capacity(channel, 1) == FVM_UNSUPPORTED);
    assert(fvm_channel_pending(channel, &pending) == FVM_UNSUPPORTED);
    fvm_channel_close(channel);
    assert(fvm_channel_receive_value(dst, channel, &other, 0) == FVM_ERROR);
    fvm_channel_destroy(channel);
    /* Default rw appends while leaving the old graph readable. */
    assert(fvm_channel_open_file(path, 0, &channel) == FVM_OK);
    assert(fvm_value_string(&value, "persisted") == FVM_OK);
    assert(fvm_channel_send(channel, &value) == FVM_OK);
    assert(fvm_channel_receive_value(dst, channel, &other, 0) == FVM_OK);
    assert(fvm_channel_receive_owned(channel, &other, 0) == FVM_OK &&
           !strcmp(other.string, "persisted"));
    fvm_channel_message_release(&other);
    fvm_channel_destroy(channel);
    assert(fvm_channel_open_file(path, "w", &channel) == FVM_OK);
    fvm_value_number(&value, -2.5);
    assert(fvm_channel_send(channel, &value) == FVM_OK);
    fvm_channel_destroy(channel);
    assert(fvm_channel_open_file(path, "r", &channel) == FVM_OK);
    assert(fvm_channel_receive(channel, &other) == FVM_OK && other.number == -2.5);
    assert(fvm_channel_available(channel, &ready) == FVM_OK && !ready);
    fvm_channel_destroy(channel);
    /* Raw bytes also reach an ordinary file without serialization framing. */
    assert(fvm_channel_open_file(raw_path, "rw", &channel) == FVM_OK);
    assert(fvm_channel_file_write(channel, "abc\0xyz", 7) == FVM_OK);
    assert(fvm_channel_file_read(channel, raw, sizeof(raw), &count) == FVM_OK && count == 7);
    assert(!memcmp(raw, "abc\0xyz", 7));
    fvm_channel_destroy(channel);
    stream = fopen(raw_path, "rb");
    assert(stream);
    assert(fread(raw, 1, sizeof(raw), stream) == 7 && !memcmp(raw, "abc\0xyz", 7));
    fclose(stream);
    assert(fvm_channel_open_file(raw_path, "r", &channel) == FVM_OK);
    assert(fvm_channel_receive_value(dst, channel, &other, 0) == FVM_INVALID_ARGUMENT);
    assert(fvm_channel_file_read(channel, raw, sizeof(raw), &count) == FVM_OK && count == 7);
    fvm_channel_destroy(channel);
    assert(fvm_gc_unpin(dst, copy.object) == FVM_OK);
    fvm_context_destroy(dst);
    fvm_runtime_destroy(runtime);
    assert(!remove(path));
    assert(!remove(raw_path));
    return 0;
}
