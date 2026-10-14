/* C89 callers can exchange retained global values without an interpreter frame. */
#include "flashvm/execution.h"
#include <assert.h>
int main(void)
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    FVM_Value value, out;
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &context) == FVM_OK);
    fvm_value_number(&value, 19);
    assert(fvm_exec_global_register(context, 1, 0, &value, &out) == FVM_OK);
    assert(out.type == FVM_VALUE_NUMBER && out.number == 19);
    assert(fvm_exec_global_register(context, 0, 0, 0, &out) == FVM_OK && out.number == 19);
    assert(fvm_exec_global_register(context, 0, FVM_EXEC_GLOBAL_REGISTER_COUNT, 0, &out) == FVM_OK);
    assert(out.type == FVM_VALUE_UNDEFINED);
    assert(fvm_exec_global_register(context, 1, 0, 0, &out) == FVM_INVALID_ARGUMENT);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    return 0;
}
