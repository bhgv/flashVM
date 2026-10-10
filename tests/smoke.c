#include "flashvm/flashvm.h"
#include <assert.h>
int main(void) {
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    assert(fvm_abi_version() == FVM_ABI_VERSION);
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &context) == FVM_OK);
    assert(fvm_advance(context, 1.0) == FVM_UNSUPPORTED);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    return 0;
}
