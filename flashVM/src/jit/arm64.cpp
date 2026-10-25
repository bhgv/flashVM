/* FlashVM: AArch64 AAPCS64 blocks with full-width helper addresses. */
#include "machine.hpp"
#include "machine_constants.h"

namespace
{


/* MOVZ/MOVK encode a zero-extended halfword and its architectural shift. */
void halfword(fvm_detail::vector<unsigned char> &b,
              unsigned long value,
              unsigned long shift,
              unsigned long reg,
              bool first,
              bool wide)
{
    fvm_jit_word(b,
                 (wide ? (first ? FVM_JIT_ARM64_MOVZ_X : FVM_JIT_ARM64_MOVK_X)
                       : (first ? FVM_JIT_ARM64_MOVZ_W : FVM_JIT_ARM64_MOVK_W)) |
                     (shift << FVM_JIT_A64_SHIFT_FIELD) | (value << FVM_JIT_A64_IMM_FIELD) | reg);
}
} // namespace


/* X19 holds state; STP/LDP preserve X19 and LR in a sixteen-byte frame. */
void fvm_jit_machine_arm64(fvm_detail::vector<unsigned char> &b,
                           FVM_JitMachineOp op,
                           const unsigned char *address,
                           unsigned int operand)
{
    if (op == FVM_JIT_MACHINE_ENTER)
    {
        fvm_jit_word(b, FVM_JIT_ARM64_BTI_C); /* BTI c is a NOP on older processors. */
        fvm_jit_word(b, FVM_JIT_ARM64_SAVE_X19_LR);
        fvm_jit_word(b, FVM_JIT_ARM64_STATE_TO_X19);
    }
    else if (op == FVM_JIT_MACHINE_LEAVE)
    {
        fvm_jit_word(b, FVM_JIT_ARM64_RESTORE_X19_LR);
        fvm_jit_word(b, FVM_JIT_ARM64_RET);
    }
    else if (op == FVM_JIT_MACHINE_CALL)
    {
        fvm_jit_word(b, FVM_JIT_ARM64_STATE_TO_X0);
        halfword(b, operand & FVM_JIT_ARM64_HALF_MASK, 0, FVM_JIT_A64_OPERAND_REG, true, false);
        halfword(b, (operand >> FVM_JIT_HALF_BITS) & FVM_JIT_ARM64_HALF_MASK, 1, FVM_JIT_A64_OPERAND_REG, false, false);
        for (size_t i = 0; i < FVM_JIT_POINTER64_BYTES / FVM_JIT_HALF_BYTES; ++i)
        {
            halfword(b,
                     address[i * FVM_JIT_HALF_BYTES] |
                         ((unsigned long)address[i * FVM_JIT_HALF_BYTES + 1] << FVM_JIT_BYTE_BITS),
                     i,
                     FVM_JIT_A64_IP0,
                     i == 0,
                     true);
        }
        fvm_jit_word(b, FVM_JIT_ARM64_BLR_X16);
    }
    else
    { /* Invert the predicate over a long-range B; no one-megabyte conditional limit. */
        if (op == FVM_JIT_MACHINE_NONZERO)
        {
            fvm_jit_word(b, FVM_JIT_ARM64_CBZ_W0_SKIP_B);
        }
        fvm_jit_word(b, FVM_JIT_ARM64_B);
    }
}
