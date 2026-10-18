/* Every ABI has an explicit pointer width; Windows LLP64 must not disable availability. */
#include "../src/jit/internal.hpp"
#include <cassert>
int main()
{
 const FVM_JitTarget narrow[]={FVM_JIT_TARGET_X86,FVM_JIT_TARGET_ARM,FVM_JIT_TARGET_RISCV};
 const FVM_JitTarget wide[]={FVM_JIT_TARGET_X64,FVM_JIT_TARGET_ARM64,FVM_JIT_TARGET_RISCV64,FVM_JIT_TARGET_WINDOWS_X64,FVM_JIT_TARGET_MACOS_X64,FVM_JIT_TARGET_MACOS_ARM64};
 for(size_t i=0;i<sizeof(narrow)/sizeof(narrow[0]);++i)assert(fvm_jit_target_width(narrow[i])==4);
 for(size_t i=0;i<sizeof(wide)/sizeof(wide[0]);++i)assert(fvm_jit_target_width(wide[i])==8);
 assert(!fvm_jit_target_width(FVM_JIT_TARGET_NONE));
 assert(!fvm_jit_target_width((FVM_JitTarget)99));
 return 0;
}
