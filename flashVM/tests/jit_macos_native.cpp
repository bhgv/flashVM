/* Native macOS OS acceptance delegates the shared, separately runnable multi-VM contract. */
#include <stdio.h>
#include <stdlib.h>
#include "../ports/mac/code_platform.h"
#include "flashvm/jit.h"
#include <cstring>

#if FVM_MAC_CODE_SUPPORTED
#define main macos_native_instances
#include "jit_native_instances.cpp"

#undef main
#endif


int main()
{
#if FVM_MAC_CODE_SUPPORTED
    if (!fvm_jit_available())
    {
        return 77;
    }
    if (!(!std::strcmp(fvm_jit_backend_name(), FVM_MAC_CODE_ARM64 ? "macos-arm64" : "macos-x64")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return macos_native_instances();
#else
    return 77;
#endif
}
