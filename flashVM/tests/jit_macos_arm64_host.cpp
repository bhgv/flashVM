/* Production selector with controlled ARM64 policy; this is not ARM64 instruction execution. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/jit/internal.hpp"
#include <cstring>

#define FVM_MAC_CODE_PLATFORM_H
#define FVM_MAC_CODE_SUPPORTED 1
#define FVM_MAC_CODE_ARM64 1
#define fvm_jit_host_target fixture_arm64_host_target
#define fvm_jit_target_width fixture_arm64_target_width
#define fvm_jit_emit fixture_arm64_emit
#define fvm_jit_backend_name fixture_arm64_backend_name
#include "../src/jit/backend.cpp"

#undef fvm_jit_host_target
#undef fvm_jit_target_width
#undef fvm_jit_emit
#undef fvm_jit_backend_name


int main()
{
#if !defined(_WIN32)
    if (!(fixture_arm64_host_target() == FVM_JIT_TARGET_MACOS_ARM64))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fixture_arm64_target_width(FVM_JIT_TARGET_MACOS_ARM64) == 8))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!std::strcmp(fixture_arm64_backend_name(), "macos-arm64")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}
