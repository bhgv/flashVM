/* FlashVM: Darwin x86-64 call blocks with a linked native frame pointer and saved invocation. */
#include "machine.hpp"
#include "macos_x64_constants.h"


/* Preserve RBP/r12 and align every helper call while keeping a conventional parent frame link. */
void fvm_jit_machine_macos_x64(fvm_detail::vector<unsigned char> &bytes,
                               FVM_JitMachineOp op,
                               const unsigned char *address,
                               unsigned int operand)
{
    static const unsigned char enter[] = {FVM_JIT_ENDBR_0,
                                          FVM_JIT_ENDBR_1,
                                          FVM_JIT_ENDBR_2,
                                          FVM_JIT_ENDBR_3,
                                          FVM_JIT_MAC64_PUSH_RBP,
                                          FVM_JIT_MAC64_REX_W,
                                          FVM_JIT_MAC64_MOV_RBP,
                                          FVM_JIT_MAC64_RBP_RSP,
                                          FVM_JIT_PUSH_R12_0,
                                          FVM_JIT_PUSH_R12_1,
                                          FVM_JIT_MAC64_REX_W,
                                          FVM_JIT_MAC64_GROUP_IMM8,
                                          FVM_JIT_MAC64_SUB_RSP,
                                          FVM_JIT_MAC64_ALIGN_BYTES,
                                          FVM_JIT_SAVE_ARG_0,
                                          FVM_JIT_SAVE_ARG_1,
                                          FVM_JIT_SAVE_ARG_2};
    static const unsigned char leave[] = {FVM_JIT_MAC64_REX_W,
                                          FVM_JIT_MAC64_GROUP_IMM8,
                                          FVM_JIT_MAC64_ADD_RSP,
                                          FVM_JIT_MAC64_ALIGN_BYTES,
                                          FVM_JIT_POP_R12_0,
                                          FVM_JIT_POP_R12_1,
                                          FVM_JIT_MAC64_POP_RBP,
                                          FVM_JIT_RET};
    if (op == FVM_JIT_MACHINE_ENTER || op == FVM_JIT_MACHINE_LEAVE)
    {
        const unsigned char *source = op == FVM_JIT_MACHINE_ENTER ? enter : leave;
        size_t size = op == FVM_JIT_MACHINE_ENTER ? sizeof(enter) : sizeof(leave);
        for (size_t i = 0; i < size; ++i)
        {
            bytes.push_back(source[i]);
        }
    }
    else if (op == FVM_JIT_MACHINE_CALL)
    {
        bytes.push_back(FVM_JIT_ARG_0);
        bytes.push_back(FVM_JIT_ARG_1);
        bytes.push_back(FVM_JIT_ARG_2);
        bytes.push_back(FVM_JIT_OPERAND);
        fvm_jit_word(bytes, operand);
        bytes.push_back(FVM_JIT_ADDRESS_0);
        bytes.push_back(FVM_JIT_ADDRESS_1);
        for (size_t i = 0; i < FVM_JIT_POINTER64_BYTES; ++i)
        {
            bytes.push_back(address[i]);
        }
        bytes.push_back(FVM_JIT_CALL_0);
        bytes.push_back(FVM_JIT_CALL_1);
    }
    else
    {
        if (op == FVM_JIT_MACHINE_NONZERO)
        {
            bytes.push_back(FVM_JIT_TEST_0);
            bytes.push_back(FVM_JIT_TEST_1);
            bytes.push_back(FVM_JIT_JNZ_0);
            bytes.push_back(FVM_JIT_JNZ_1);
        }
        else
        {
            bytes.push_back(FVM_JIT_JMP);
        }
        fvm_jit_word(bytes, 0);
    }
}
