/* Every ABI has an explicit pointer width; Windows LLP64 must not disable availability. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/jit/internal.hpp"


int main()
{
    const FVM_JitTarget narrow[] = {FVM_JIT_TARGET_X86, FVM_JIT_TARGET_ARM, FVM_JIT_TARGET_RISCV};
    const FVM_JitTarget wide[] = {FVM_JIT_TARGET_X64,
                                  FVM_JIT_TARGET_ARM64,
                                  FVM_JIT_TARGET_RISCV64,
                                  FVM_JIT_TARGET_WINDOWS_X64,
                                  FVM_JIT_TARGET_MACOS_X64,
                                  FVM_JIT_TARGET_MACOS_ARM64};
    for (size_t i = 0; i < sizeof(narrow) / sizeof(narrow[0]); ++i)
    {
        if (!(fvm_jit_target_width(narrow[i]) == 4))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    for (size_t i = 0; i < sizeof(wide) / sizeof(wide[0]); ++i)
    {
        if (!(fvm_jit_target_width(wide[i]) == 8))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(!fvm_jit_target_width(FVM_JIT_TARGET_NONE)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!fvm_jit_target_width((FVM_JitTarget)99)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
