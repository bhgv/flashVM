/* FlashVM: i386 System V cdecl blocks with aligned outgoing argument storage. */
#include "machine.hpp"
#include "machine_constants.h"


/* Keep state in callee-saved ESI; ESP is 16-byte aligned immediately before every call. */
void fvm_jit_machine_x86(fvm_detail::vector<unsigned char> &b,
                         FVM_JitMachineOp op,
                         const unsigned char *address,
                         unsigned int operand)
{
    static const unsigned char enter[] = {FVM_JIT_X86_ENDBR_PREFIX,
                                          FVM_JIT_X86_ESCAPE,
                                          FVM_JIT_X86_ENDBR_OPCODE,
                                          FVM_JIT_X86_ENDBR32_SUFFIX,
                                          FVM_JIT_X86_PUSH_ESI,
                                          FVM_JIT_X86_GROUP1_IMM8,
                                          FVM_JIT_X86_SUB_ESP,
                                          FVM_JIT_X86_FRAME_BYTES,
                                          FVM_JIT_X86_MOV_FROM_RM,
                                          FVM_JIT_X86_ESI_SIB_DISP8,
                                          FVM_JIT_X86_ESP_SIB,
                                          FVM_JIT_X86_STATE_STACK_OFFSET};
    static const unsigned char args[] = {FVM_JIT_X86_MOV_TO_RM,
                                         FVM_JIT_X86_ESI_SIB,
                                         FVM_JIT_X86_ESP_SIB,
                                         FVM_JIT_X86_MOV_IMM_RM,
                                         FVM_JIT_X86_ARG_SIB_DISP8,
                                         FVM_JIT_X86_ESP_SIB,
                                         FVM_JIT_X86_OPERAND_STACK_OFFSET};
    static const unsigned char leave[] = {
        FVM_JIT_X86_GROUP1_IMM8, FVM_JIT_X86_ADD_ESP, FVM_JIT_X86_FRAME_BYTES, FVM_JIT_X86_POP_ESI, FVM_JIT_X86_RET};
    if (op == FVM_JIT_MACHINE_ENTER || op == FVM_JIT_MACHINE_LEAVE)
    {
        const unsigned char *p = op == FVM_JIT_MACHINE_ENTER ? enter : leave;
        size_t n = op == FVM_JIT_MACHINE_ENTER ? sizeof(enter) : sizeof(leave);
        for (size_t i = 0; i < n; ++i)
        {
            b.push_back(p[i]);
        }
    }
    else if (op == FVM_JIT_MACHINE_CALL)
    {
        for (size_t i = 0; i < sizeof(args); ++i)
        {
            b.push_back(args[i]);
        }
        fvm_jit_word(b, operand);
        b.push_back(FVM_JIT_X86_MOV_EAX_IMM);
        for (size_t i = 0; i < FVM_JIT_POINTER32_BYTES; ++i)
        {
            b.push_back(address[i]);
        }
        b.push_back(FVM_JIT_X86_GROUP5);
        b.push_back(FVM_JIT_X86_CALL_EAX);
    }
    else
    {
        if (op == FVM_JIT_MACHINE_NONZERO)
        {
            b.push_back(FVM_JIT_X86_TEST_OR_JNZ);
            b.push_back(FVM_JIT_X86_EAX_EAX);
            b.push_back(FVM_JIT_X86_ESCAPE);
            b.push_back(FVM_JIT_X86_TEST_OR_JNZ);
        }
        else
        {
            b.push_back(FVM_JIT_X86_JMP_REL32);
        }
        fvm_jit_word(b, 0);
    }
}
