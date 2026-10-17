/* FlashVM: native ABI selection and W^X publication for architecture-specific encoders. */
#include "internal.hpp"
#include <cstring>
/* Reject big-endian, x32, AArch64 ILP32, RV32E and pre-ARMv7 configurations explicitly. */
FVM_JitTarget fvm_jit_host_target(void)
{
#if defined(_WIN32) && (defined(_M_X64) || defined(__x86_64__)) && !defined(__ILP32__) && \
    !defined(_M_ARM64EC) && !defined(__arm64ec__)
    return FVM_JIT_TARGET_WINDOWS_X64;
#elif !defined(__linux__) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__)
    return FVM_JIT_TARGET_NONE;
#elif defined(__x86_64__) && !defined(__ILP32__)
    return FVM_JIT_TARGET_X64;
#elif defined(__i386__)
    return FVM_JIT_TARGET_X86;
#elif defined(__aarch64__) && !defined(__ILP32__)
    return FVM_JIT_TARGET_ARM64;
#elif defined(__arm__) && defined(__ARM_ARCH) && __ARM_ARCH >= FVM_JIT_ARM_MIN_ARCH &&             \
    !defined(__ARMEB__) && (!defined(__ARM_ARCH_PROFILE) || __ARM_ARCH_PROFILE != 'M')
    return FVM_JIT_TARGET_ARM;
#elif defined(__riscv) && !defined(__riscv_abi_rve) && __riscv_xlen == FVM_JIT_POINTER32_BITS
    return FVM_JIT_TARGET_RISCV;
#elif defined(__riscv) && !defined(__riscv_abi_rve) && __riscv_xlen == FVM_JIT_POINTER64_BITS
    return FVM_JIT_TARGET_RISCV64;
#else
    return FVM_JIT_TARGET_NONE;
#endif
}
/* Pointer sizes are ABI properties, not a way to infer an instruction set. */
size_t fvm_jit_target_width(FVM_JitTarget target)
{
    switch (target)
    {
    case FVM_JIT_TARGET_X86:
    case FVM_JIT_TARGET_ARM:
    case FVM_JIT_TARGET_RISCV:
        return FVM_JIT_POINTER32_BYTES;
    case FVM_JIT_TARGET_X64:
    case FVM_JIT_TARGET_WINDOWS_X64:
    case FVM_JIT_TARGET_MACOS_X64:
    case FVM_JIT_TARGET_ARM64:
    case FVM_JIT_TARGET_RISCV64:
        return FVM_JIT_POINTER64_BYTES;
    default:
        return 0;
    }
}
namespace
{
/* Copy a native function representation only for a matching-width host encoder. */
bool resolve(FVM_JitStep step, unsigned char *address, void *)
{
    if (sizeof(step) > FVM_JIT_POINTER64_BYTES)
        return false;
    std::memset(address, 0, FVM_JIT_POINTER64_BYTES);
    std::memcpy(address, &step, sizeof(step));
    return true;
}
} // namespace
/* Resolve live helpers only for the native ABI, then publish sealed instruction memory. */
FVM_Result fvm_jit_emit(FVM_JitCode &code)
{
    FVM_JitTarget target = fvm_jit_host_target();
    if (target == FVM_JIT_TARGET_X64)
        return fvm_jit_emit_x64(code);
    fvm_detail::vector<unsigned char> bytes;
    FVM_Result status = fvm_jit_encode(target, code, bytes, resolve, 0);
    if (status != FVM_OK)
        return status;
    status = fvm_port_code_create(bytes.begin(), bytes.size(), &code.memory, &code.mapped);
    if (status == FVM_OK)
        code.bytes = bytes.size();
    return status;
}

/* Stable architecture names let one embedding example run on every native build. */
extern "C" const char *fvm_jit_backend_name(void)
{
    switch (fvm_jit_host_target())
    {
    case FVM_JIT_TARGET_WINDOWS_X64:
        return "windows-x64";
    case FVM_JIT_TARGET_X86:
        return "x86";
    case FVM_JIT_TARGET_X64:
        return "x86-64";
    case FVM_JIT_TARGET_ARM:
        return "armv7-a32";
    case FVM_JIT_TARGET_ARM64:
        return "arm64";
    case FVM_JIT_TARGET_RISCV:
        return "riscv32";
    case FVM_JIT_TARGET_RISCV64:
        return "riscv64";
    default:
        return "unsupported";
    }
}
