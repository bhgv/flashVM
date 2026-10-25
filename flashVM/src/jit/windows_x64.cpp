/* FlashVM: Microsoft x64 machine blocks with fixed shadow space and unwindable prologue. */
#include "machine.hpp"
#include "windows_x64_constants.h"


/* Preserve only r12, align helper calls and reserve all four volatile register home slots. */
void fvm_jit_machine_windows_x64(fvm_detail::vector<unsigned char> &bytes,
                                 FVM_JitMachineOp op,
                                 const unsigned char *address,
                                 unsigned int operand)
{
    static const unsigned char enter[] = {FVM_JIT_ENDBR_0,
                                          FVM_JIT_ENDBR_1,
                                          FVM_JIT_ENDBR_2,
                                          FVM_JIT_ENDBR_3,
                                          FVM_JIT_PUSH_R12_0,
                                          FVM_JIT_PUSH_R12_1,
                                          FVM_JIT_WIN64_REX_W,
                                          FVM_JIT_WIN64_GROUP_IMM8,
                                          FVM_JIT_WIN64_SUB_RSP,
                                          FVM_JIT_WIN64_SHADOW_BYTES,
                                          FVM_JIT_SAVE_ARG_0,
                                          FVM_JIT_SAVE_ARG_1,
                                          FVM_JIT_WIN64_SAVE_RCX};
    static const unsigned char leave[] = {FVM_JIT_WIN64_REX_W,
                                          FVM_JIT_WIN64_GROUP_IMM8,
                                          FVM_JIT_WIN64_ADD_RSP,
                                          FVM_JIT_WIN64_SHADOW_BYTES,
                                          FVM_JIT_POP_R12_0,
                                          FVM_JIT_POP_R12_1,
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
        bytes.push_back(FVM_JIT_WIN64_ARG_RCX);
        bytes.push_back(FVM_JIT_WIN64_OPERAND_EDX);
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
