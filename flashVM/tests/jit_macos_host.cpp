/* Compile actual host selection with a controlled desktop Darwin policy on an x64 host. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/jit/internal.hpp"
#include <cstring>

#if defined(__x86_64__) && !defined(__ILP32__) && !defined(_WIN32)
#define FVM_MAC_CODE_PLATFORM_H
#define FVM_MAC_CODE_SUPPORTED 1
#define fvm_jit_host_target fixture_macos_host_target
#define fvm_jit_target_width fixture_macos_target_width
#define fvm_jit_emit fixture_macos_emit
#define fvm_jit_backend_name fixture_macos_backend_name
#include "../src/jit/backend.cpp"

#undef fvm_jit_host_target
#undef fvm_jit_target_width
#undef fvm_jit_emit
#undef fvm_jit_backend_name
#endif


int main()
{
#if defined(__x86_64__) && !defined(__ILP32__) && !defined(_WIN32)
    FVM_JitTarget selected = fixture_macos_host_target();
    if (!(selected == FVM_JIT_TARGET_MACOS_X64))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!std::strcmp(fixture_macos_backend_name(), "macos-x64")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fixture_macos_target_width(selected) == 8 && fvm_jit_target_width(selected) == sizeof(void *)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}
