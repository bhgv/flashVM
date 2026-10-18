/* Production selector with controlled ARM64 policy; this is not ARM64 instruction execution. */
#include "../src/jit/internal.hpp"
#include <cassert>
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
    assert(fixture_arm64_host_target()==FVM_JIT_TARGET_MACOS_ARM64);
    assert(fixture_arm64_target_width(FVM_JIT_TARGET_MACOS_ARM64)==8);
    assert(!std::strcmp(fixture_arm64_backend_name(),"macos-arm64"));
#endif
    return 0;
}
