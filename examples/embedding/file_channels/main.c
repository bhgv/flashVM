#include "flashvm/transfer.h"
#include "flashvm/table.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
/* The receiver can be a new VM or a later process opening the same file. */
int main(void)
{
    FVM_Config config;
    FVM_Runtime *runtime;
    FVM_Context *writer, *reader;
    FVM_Channel *file;
    FVM_Object *object;
    FVM_Value value, restored, field;
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &writer) == FVM_OK);
    assert(fvm_object_create(writer, &object) == FVM_OK);
    assert(fvm_value_string(&value, "Hello from FlashVM") == FVM_OK);
    assert(fvm_object_set(writer, object, "message", &value) == FVM_OK);
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = object;
    assert(fvm_object_set(writer, object, "self", &value) == FVM_OK);
    assert(fvm_channel_open_file("flashvm-example.bin", "w", &file) == FVM_OK);
    assert(fvm_channel_send_value(writer, file, &value, 1) == FVM_OK);
    fvm_channel_destroy(file);
    fvm_context_destroy(writer);
    assert(fvm_context_create(runtime, &reader) == FVM_OK);
    assert(fvm_channel_open_file("flashvm-example.bin", "r", &file) == FVM_OK);
    assert(fvm_channel_receive_value(reader, file, &restored, 1) == FVM_OK);
    assert(fvm_object_get(reader, restored.object, "self", &field) == FVM_OK &&
           field.object == restored.object);
    assert(fvm_object_get(reader, restored.object, "message", &field) == FVM_OK);
    assert(!strcmp(field.string, "Hello from FlashVM"));
    fvm_channel_destroy(file);
    fvm_context_destroy(reader);
    fvm_runtime_destroy(runtime);
    assert(!remove("flashvm-example.bin"));
    return 0;
}
