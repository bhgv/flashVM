#include "flashvm/transfer.h"
#include "flashvm/standard.h"
#include "flashvm/table.h"
#include <assert.h>
int main(void)
{
    FVM_Config config;
    FVM_Runtime *runtime;
    FVM_Context *context;
    FVM_Channel *file;
    FVM_Object *boxed, *meta;
    FVM_Value value;
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &context) == FVM_OK);
    assert(fvm_standard_install(context) == FVM_OK);
    fvm_value_boolean(&value, 0);
    assert(fvm_construct(context, "Boolean", 1, &value, &boxed) == FVM_OK);
    assert(fvm_object_create(context, &meta) == FVM_OK);
    assert(fvm_table_set_metatable(context, boxed, meta) == FVM_OK);
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = boxed;
    assert(fvm_object_set(context, boxed, "self", &value) == FVM_OK);
    assert(fvm_channel_open_file("process-state.bin", "w", &file) == FVM_OK);
    assert(fvm_channel_send_value(context, file, &value, 0) == FVM_OK);
    assert(fvm_get_global(context, "Math", &value) == FVM_OK);
    assert(fvm_channel_send_value(context, file, &value, 0) == FVM_OK);
    fvm_channel_destroy(file);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    /* The separate reader executable checks this file after this process exits. */
    return 0;
}
