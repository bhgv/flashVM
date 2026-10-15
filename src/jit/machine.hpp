/* FlashVM: fixed-width instruction encoders and shared native branch relocation. */
#ifndef FVM_JIT_MACHINE_HPP
#define FVM_JIT_MACHINE_HPP /* Machine lowering helper guard. */
#include "internal.hpp"
/* Operations supplied to each architecture's small instruction encoder. */
enum FVM_JitMachineOp
{
    FVM_JIT_MACHINE_ENTER,
    FVM_JIT_MACHINE_CALL,
    FVM_JIT_MACHINE_JUMP,
    FVM_JIT_MACHINE_NONZERO,
    FVM_JIT_MACHINE_LEAVE
};
/* Append a little-endian 32-bit instruction or immediate. */
void fvm_jit_word(fvm_detail::vector<unsigned char> &, unsigned long);
/* Architecture-specific lowering; jump offsets are patched by the common driver. */
void fvm_jit_machine_x86(fvm_detail::vector<unsigned char> &, FVM_JitMachineOp,
                         const unsigned char *, unsigned int);
/* ARMv7 A32 with AAPCS and interworking helper calls. */
void fvm_jit_machine_arm(fvm_detail::vector<unsigned char> &, FVM_JitMachineOp,
                         const unsigned char *, unsigned int);
/* AArch64 AAPCS64 integer-call lowering. */
void fvm_jit_machine_arm64(fvm_detail::vector<unsigned char> &, FVM_JitMachineOp,
                           const unsigned char *, unsigned int);
/* RV32I/RV64I standard psABI; no compressed instructions or extension dependency. */
void fvm_jit_machine_riscv(fvm_detail::vector<unsigned char> &, FVM_JitMachineOp,
                           const unsigned char *, unsigned int, bool);
#endif
