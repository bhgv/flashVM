/* FlashVM: documented native instruction encodings; values follow the ISA manuals. */
#ifndef FVM_JIT_MACHINE_CONSTANTS_H
/* Encoder field definitions. */
#define FVM_JIT_MACHINE_CONSTANTS_H
/* endbr prefix. */
#define FVM_JIT_X86_ENDBR_PREFIX 0xf3U
/* escape. */
#define FVM_JIT_X86_ESCAPE 0x0fU
/* endbr opcode. */
#define FVM_JIT_X86_ENDBR_OPCODE 0x1eU
/* endbr32 suffix. */
#define FVM_JIT_X86_ENDBR32_SUFFIX 0xfbU
/* push esi. */
#define FVM_JIT_X86_PUSH_ESI 0x56U
/* group1 imm8. */
#define FVM_JIT_X86_GROUP1_IMM8 0x83U
/* sub esp. */
#define FVM_JIT_X86_SUB_ESP 0xecU
/* frame bytes. */
#define FVM_JIT_X86_FRAME_BYTES 0x08U
/* mov from rm. */
#define FVM_JIT_X86_MOV_FROM_RM 0x8bU
/* esi sib disp8. */
#define FVM_JIT_X86_ESI_SIB_DISP8 0x74U
/* esp sib. */
#define FVM_JIT_X86_ESP_SIB 0x24U
/* state stack offset. */
#define FVM_JIT_X86_STATE_STACK_OFFSET 0x10U
/* mov to rm. */
#define FVM_JIT_X86_MOV_TO_RM 0x89U
/* esi sib. */
#define FVM_JIT_X86_ESI_SIB 0x34U
/* mov imm rm. */
#define FVM_JIT_X86_MOV_IMM_RM 0xc7U
/* arg sib disp8. */
#define FVM_JIT_X86_ARG_SIB_DISP8 0x44U
/* operand stack offset. */
#define FVM_JIT_X86_OPERAND_STACK_OFFSET 0x04U
/* add esp. */
#define FVM_JIT_X86_ADD_ESP 0xc4U
/* pop esi. */
#define FVM_JIT_X86_POP_ESI 0x5eU
/* ret. */
#define FVM_JIT_X86_RET 0xc3U
/* mov eax imm. */
#define FVM_JIT_X86_MOV_EAX_IMM 0xb8U
/* group5. */
#define FVM_JIT_X86_GROUP5 0xffU
/* call eax. */
#define FVM_JIT_X86_CALL_EAX 0xd0U
/* test or jnz. */
#define FVM_JIT_X86_TEST_OR_JNZ 0x85U
/* eax eax. */
#define FVM_JIT_X86_EAX_EAX 0xc0U
/* jmp rel32. */
#define FVM_JIT_X86_JMP_REL32 0xe9U
/* half mask. */
#define FVM_JIT_ARM_HALF_MASK 0xffffUL
/* movw. */
#define FVM_JIT_ARM_MOVW 0xe3000000UL
/* imm high mask. */
#define FVM_JIT_ARM_IMM_HIGH_MASK 0xf000UL
/* imm low mask. */
#define FVM_JIT_ARM_IMM_LOW_MASK 0xfffUL
/* movt. */
#define FVM_JIT_ARM_MOVT 0xe3400000UL
/* push r4 lr. */
#define FVM_JIT_ARM_PUSH_R4_LR 0xe92d4010UL
/* state to r4. */
#define FVM_JIT_ARM_STATE_TO_R4 0xe1a04000UL
/* pop r4 pc. */
#define FVM_JIT_ARM_POP_R4_PC 0xe8bd8010UL
/* state to r0. */
#define FVM_JIT_ARM_STATE_TO_R0 0xe1a00004UL
/* blx ip. */
#define FVM_JIT_ARM_BLX_IP 0xe12fff3cUL
/* cmp status zero. */
#define FVM_JIT_ARM_CMP_STATUS_ZERO 0xe3500000UL
/* bne. */
#define FVM_JIT_ARM_BNE 0x1a000000UL
/* b. */
#define FVM_JIT_ARM_B 0xea000000UL
/* movz x. */
#define FVM_JIT_ARM64_MOVZ_X 0xd2800000UL
/* movk x. */
#define FVM_JIT_ARM64_MOVK_X 0xf2800000UL
/* movz w. */
#define FVM_JIT_ARM64_MOVZ_W 0x52800000UL
/* movk w. */
#define FVM_JIT_ARM64_MOVK_W 0x72800000UL
/* bti c. */
#define FVM_JIT_ARM64_BTI_C 0xd503245fUL
/* save x19 lr. */
#define FVM_JIT_ARM64_SAVE_X19_LR 0xa9bf7bf3UL
/* state to x19. */
#define FVM_JIT_ARM64_STATE_TO_X19 0xaa0003f3UL
/* restore x19 lr. */
#define FVM_JIT_ARM64_RESTORE_X19_LR 0xa8c17bf3UL
/* ret. */
#define FVM_JIT_ARM64_RET 0xd65f03c0UL
/* state to x0. */
#define FVM_JIT_ARM64_STATE_TO_X0 0xaa1303e0UL
/* half mask. */
#define FVM_JIT_ARM64_HALF_MASK 0xffffUL
/* blr x16. */
#define FVM_JIT_ARM64_BLR_X16 0xd63f0200UL
/* cbz w0 skip b. */
#define FVM_JIT_ARM64_CBZ_W0_SKIP_B 0x34000040UL
/* b. */
#define FVM_JIT_ARM64_B 0x14000000UL
/* alloc frame. */
#define FVM_JIT_RISCV_ALLOC_FRAME 0xff010113UL
/* save ra64. */
#define FVM_JIT_RISCV_SAVE_RA64 0x00113423UL
/* save ra32. */
#define FVM_JIT_RISCV_SAVE_RA32 0x00112623UL
/* save s064. */
#define FVM_JIT_RISCV_SAVE_S064 0x00813023UL
/* save s032. */
#define FVM_JIT_RISCV_SAVE_S032 0x00812423UL
/* state to s0. */
#define FVM_JIT_RISCV_STATE_TO_S0 0x00050413UL
/* restore ra64. */
#define FVM_JIT_RISCV_RESTORE_RA64 0x00813083UL
/* restore ra32. */
#define FVM_JIT_RISCV_RESTORE_RA32 0x00c12083UL
/* restore s064. */
#define FVM_JIT_RISCV_RESTORE_S064 0x00013403UL
/* restore s032. */
#define FVM_JIT_RISCV_RESTORE_S032 0x00812403UL
/* free frame. */
#define FVM_JIT_RISCV_FREE_FRAME 0x01010113UL
/* ret. */
#define FVM_JIT_RISCV_RET 0x00008067UL
/* state to a0. */
#define FVM_JIT_RISCV_STATE_TO_A0 0x00040513UL
/* imm round. */
#define FVM_JIT_RISCV_IMM_ROUND 0x800UL
/* high mask. */
#define FVM_JIT_RISCV_HIGH_MASK 0xfffff000UL
/* lui a1. */
#define FVM_JIT_RISCV_LUI_A1 0x5b7UL
/* low mask. */
#define FVM_JIT_RISCV_LOW_MASK 0xfffUL
/* addi a1. */
#define FVM_JIT_RISCV_ADDI_A1 0x58593UL
/* nop. */
#define FVM_JIT_RISCV_NOP 0x00000013UL
/* auipc t0. */
#define FVM_JIT_RISCV_AUIPC_T0 0x00000297UL
/* load literal64. */
#define FVM_JIT_RISCV_LOAD_LITERAL64 0x0102b283UL
/* load literal32. */
#define FVM_JIT_RISCV_LOAD_LITERAL32 0x0102a283UL
/* call t0. */
#define FVM_JIT_RISCV_CALL_T0 0x000280e7UL
/* skip literal64. */
#define FVM_JIT_RISCV_SKIP_LITERAL64 0x00c0006fUL
/* skip literal32. */
#define FVM_JIT_RISCV_SKIP_LITERAL32 0x0080006fUL
/* beq status skip. */
#define FVM_JIT_RISCV_BEQ_STATUS_SKIP 0x00050663UL
/* jump t0. */
#define FVM_JIT_RISCV_JUMP_T0 0x00028067UL
/* RV64 add/sign-extend low 32-bit operand. */
#define FVM_JIT_RISCV_ADDIW_A1 0x0005859bUL
/* Bytes per immediate halfword. */
#define FVM_JIT_HALF_BYTES 2U
/* AAPCS second integer argument, R1. */
#define FVM_JIT_ARM_OPERAND_REG 1U
/* AAPCS64 second integer argument, W1. */
#define FVM_JIT_A64_OPERAND_REG 1U
/* Bits per MOVW/MOVZ immediate chunk. */
#define FVM_JIT_HALF_BITS 16U
/* Move A32 imm[15:12] into instruction bits [19:16]. */
#define FVM_JIT_ARM_IMM_SPLIT_SHIFT 4U
/* A32 destination register bit position. */
#define FVM_JIT_ARM_REGISTER_SHIFT 12U
/* AAPCS inter-procedure scratch register. */
#define FVM_JIT_ARM_IP 12U
/* A64 MOVZ/MOVK halfword-selector bit position. */
#define FVM_JIT_A64_SHIFT_FIELD 21U
/* A64 MOVZ/MOVK immediate bit position. */
#define FVM_JIT_A64_IMM_FIELD 5U
/* AAPCS64 inter-procedure scratch register. */
#define FVM_JIT_A64_IP0 16U
#endif
