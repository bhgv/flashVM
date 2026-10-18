/* FlashVM: RV32I and RV64I Linux psABI call blocks, without optional ISA extensions. */
#include "machine.hpp"
#include "machine_constants.h"
/* S0 retains state; preserve S0/RA and sixteen-byte stack alignment on both ABIs. */
void fvm_jit_machine_riscv(fvm_detail::vector<unsigned char> &b, FVM_JitMachineOp op,
                           const unsigned char *address, unsigned int operand, bool wide)
{
    if (op == FVM_JIT_MACHINE_ENTER)
    {
        fvm_jit_word(b, FVM_JIT_RISCV_ALLOC_FRAME);
        fvm_jit_word(b, wide ? FVM_JIT_RISCV_SAVE_RA64 : FVM_JIT_RISCV_SAVE_RA32);
        fvm_jit_word(b, wide ? FVM_JIT_RISCV_SAVE_S064 : FVM_JIT_RISCV_SAVE_S032);
        fvm_jit_word(b, FVM_JIT_RISCV_STATE_TO_S0);
    }
    else if (op == FVM_JIT_MACHINE_LEAVE)
    {
        fvm_jit_word(b, wide ? FVM_JIT_RISCV_RESTORE_RA64 : FVM_JIT_RISCV_RESTORE_RA32);
        fvm_jit_word(b, wide ? FVM_JIT_RISCV_RESTORE_S064 : FVM_JIT_RISCV_RESTORE_S032);
        fvm_jit_word(b, FVM_JIT_RISCV_FREE_FRAME);
        fvm_jit_word(b, FVM_JIT_RISCV_RET);
    }
    else if (op == FVM_JIT_MACHINE_CALL)
    {
        fvm_jit_word(b, FVM_JIT_RISCV_STATE_TO_A0);
        /* LUI plus ADDIW on RV64 preserves the psABI-required sign extension of 32-bit arguments.
         */
        unsigned long value = operand;
        fvm_jit_word(b, ((value + FVM_JIT_RISCV_IMM_ROUND) & FVM_JIT_RISCV_HIGH_MASK) |
                            FVM_JIT_RISCV_LUI_A1);
        fvm_jit_word(b, ((value & FVM_JIT_RISCV_LOW_MASK) << FVM_JIT_RV_IMMEDIATE_SHIFT) |
                            (wide ? FVM_JIT_RISCV_ADDIW_A1 : FVM_JIT_RISCV_ADDI_A1));
        if (wide && b.size() % FVM_JIT_POINTER64_BYTES)
            fvm_jit_word(b, FVM_JIT_RISCV_NOP);
        /* PC-relative literal load avoids RV64 absolute-address sign/truncation hazards. */
        fvm_jit_word(b, FVM_JIT_RISCV_AUIPC_T0);
        fvm_jit_word(b, wide ? FVM_JIT_RISCV_LOAD_LITERAL64 : FVM_JIT_RISCV_LOAD_LITERAL32);
        fvm_jit_word(b, FVM_JIT_RISCV_CALL_T0);
        fvm_jit_word(b, wide ? FVM_JIT_RISCV_SKIP_LITERAL64 : FVM_JIT_RISCV_SKIP_LITERAL32);
        size_t width = wide ? FVM_JIT_POINTER64_BYTES : FVM_JIT_POINTER32_BYTES;
        for (size_t i = 0; i < width; ++i)
            b.push_back(address[i]);
    }
    else
    {
        /* BEQ a0,zero,+12 skips a full-range AUIPC/JALR branch veneer. */
        if (op == FVM_JIT_MACHINE_NONZERO)
            fvm_jit_word(b, FVM_JIT_RISCV_BEQ_STATUS_SKIP);
        fvm_jit_word(b, FVM_JIT_RISCV_AUIPC_T0);
        fvm_jit_word(b, FVM_JIT_RISCV_JUMP_T0);
    }
}
