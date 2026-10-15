/* FlashVM: ARMv7 A32 AAPCS blocks; BLX honors Thumb-tagged helper addresses. */
#include "machine.hpp"
#include "machine_constants.h"
namespace
{
/* Materialize one complete unsigned 32-bit value using MOVW/MOVT. */
void immediate(fvm_detail::vector<unsigned char> &b, unsigned long value, unsigned long reg)
{
    unsigned long low = value & FVM_JIT_ARM_HALF_MASK,
                  high = (value >> FVM_JIT_HALF_BITS) & FVM_JIT_ARM_HALF_MASK;
    fvm_jit_word(b, FVM_JIT_ARM_MOVW |
                        ((low & FVM_JIT_ARM_IMM_HIGH_MASK) << FVM_JIT_ARM_IMM_SPLIT_SHIFT) |
                        (reg << FVM_JIT_ARM_REGISTER_SHIFT) | (low & FVM_JIT_ARM_IMM_LOW_MASK));
    fvm_jit_word(b, FVM_JIT_ARM_MOVT |
                        ((high & FVM_JIT_ARM_IMM_HIGH_MASK) << FVM_JIT_ARM_IMM_SPLIT_SHIFT) |
                        (reg << FVM_JIT_ARM_REGISTER_SHIFT) | (high & FVM_JIT_ARM_IMM_LOW_MASK));
}
} // namespace
/* Save R4/LR in an eight-byte frame; helper arguments use R0/R1 and status uses R0. */
void fvm_jit_machine_arm(fvm_detail::vector<unsigned char> &b, FVM_JitMachineOp op,
                         const unsigned char *address, unsigned int operand)
{
    if (op == FVM_JIT_MACHINE_ENTER)
    {
        fvm_jit_word(b, FVM_JIT_ARM_PUSH_R4_LR);
        fvm_jit_word(b, FVM_JIT_ARM_STATE_TO_R4);
    }
    else if (op == FVM_JIT_MACHINE_LEAVE)
        fvm_jit_word(b, FVM_JIT_ARM_POP_R4_PC);
    else if (op == FVM_JIT_MACHINE_CALL)
    {
        fvm_jit_word(b, FVM_JIT_ARM_STATE_TO_R0);
        immediate(b, operand, FVM_JIT_ARM_OPERAND_REG);
        unsigned long value = 0;
        for (size_t i = 0; i < FVM_JIT_POINTER32_BYTES; ++i)
            value |= (unsigned long)address[i] << (i * FVM_JIT_BYTE_BITS);
        immediate(b, value, FVM_JIT_ARM_IP);
        fvm_jit_word(b, FVM_JIT_ARM_BLX_IP);
    }
    else
    {
        if (op == FVM_JIT_MACHINE_NONZERO)
            fvm_jit_word(b, FVM_JIT_ARM_CMP_STATUS_ZERO);
        fvm_jit_word(b, op == FVM_JIT_MACHINE_NONZERO ? FVM_JIT_ARM_BNE : FVM_JIT_ARM_B);
    }
}
