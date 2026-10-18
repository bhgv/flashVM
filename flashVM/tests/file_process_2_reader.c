#include "flashvm/transfer.h"
#include "flashvm/table.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    FVM_Config config;
    FVM_Runtime *runtime;
    FVM_Context *context;
    FVM_Channel *file;
    FVM_Object *meta;
    FVM_Value value, out, arg;
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &context) == FVM_OK);
    assert(fvm_channel_open_file("process-state.bin", "r", &file) == FVM_OK);
    assert(fvm_channel_receive_value(context, file, &value, 0) == FVM_OK);
    assert(fvm_object_get(context, value.object, "self", &out) == FVM_OK &&
           out.object == value.object);
    assert(fvm_table_get_metatable(context, value.object, &meta) == FVM_OK && meta);
    assert(fvm_object_call(context, value.object, "valueOf", 0, 0, &out) == FVM_OK && !out.boolean);
    assert(fvm_channel_receive_value(context, file, &value, 0) == FVM_OK);
    fvm_value_number(&arg, -7);
    assert(fvm_object_call(context, value.object, "abs", 1, &arg, &out) == FVM_OK &&
           out.number == 7);
    assert(fvm_object_call(context, value.object, "random", 0, 0, &out) == FVM_OK &&
           out.number >= 0 && out.number < 1);
    assert(fvm_channel_receive_value(context, file, &value, 0) == FVM_NOT_FOUND);
    fvm_channel_destroy(file);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    assert(!remove("process-state.bin"));
    return 0;
}
