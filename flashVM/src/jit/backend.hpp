/* FlashVM: host selection and offline, address-resolved architecture encoders. */
#ifndef FVM_JIT_BACKEND_HPP
#define FVM_JIT_BACKEND_HPP /* Private cross-emission interface. */
#include "flashvm/jit.h"
#include "../details/vector.hpp"


/* Each target names an instruction set and its explicit OS calling convention. */
enum FVM_JitTarget
{
    FVM_JIT_TARGET_NONE,
    FVM_JIT_TARGET_X86,
    FVM_JIT_TARGET_ARM,
    FVM_JIT_TARGET_ARM64,
    FVM_JIT_TARGET_RISCV,
    FVM_JIT_TARGET_RISCV64,
    FVM_JIT_TARGET_X64,
    FVM_JIT_TARGET_WINDOWS_X64, /* Microsoft x64 integer ABI, separate from System V. */
    FVM_JIT_TARGET_MACOS_X64,   /* Darwin x86-64 integer ABI with a linked RBP frame. */
    FVM_JIT_TARGET_MACOS_ARM64  /* Offline Darwin ARM64 integer ABI; no memory policy implied. */
};
struct FVM_JitCode;
struct FVM_JitInvocation;


/* Resolve a helper to eight little-endian bytes; foreign encoders never truncate host pointers. */
typedef bool (*FVM_JitResolver)(FVM_Result (*)(FVM_JitInvocation *, unsigned int), unsigned char *, void *);


/* Return the native build ABI, or NONE for deliberately unsupported variants. */
FVM_JitTarget fvm_jit_host_target(void);


/* Return the pointer width required by a target ABI. */
size_t fvm_jit_target_width(FVM_JitTarget);
/* Generate foreign or native bytes without mapping executable memory. */
FVM_Result
fvm_jit_encode(FVM_JitTarget, const FVM_JitCode &, fvm_detail::vector<unsigned char> &, FVM_JitResolver, void *);


/* Existing Linux System V AMD64 emitter. */
FVM_Result fvm_jit_emit_x64(FVM_JitCode &);
#endif
