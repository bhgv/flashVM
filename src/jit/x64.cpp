/* FlashVM: System V x86-64 call-block emitter with native jumps and a shared status epilogue. */
#include "internal.hpp"
#include <cstring>
namespace
{
/* One relative displacement to patch after native block labels are known. */
struct Fixup
{
    size_t at;    /* First byte of the relative displacement. */
    size_t label; /* Action index, or the common-exit sentinel. */
    /* Initialize a relocation or an unused capacity slot. */
    Fixup(size_t a = 0, size_t l = 0) : at(a), label(l)
    {
    }
};
/* Append little-endian imm32/rel32 bits without signed integer overflow. */
void word(fvm_detail::vector<unsigned char> &bytes, unsigned long value)
{
    for (unsigned int i = 0; i < FVM_JIT_IMMEDIATE_BYTES; ++i)
        bytes.push_back((unsigned char)((value >> (i * FVM_JIT_BYTE_BITS)) & FVM_JIT_BYTE_MASK));
}
/* Pass the rooted state and semantic operand, then call the compile-time selected safe helper. */
void call(fvm_detail::vector<unsigned char> &bytes, FVM_JitStep step, unsigned int operand)
{
    const unsigned char argument[] = {FVM_JIT_ARG_0, FVM_JIT_ARG_1, FVM_JIT_ARG_2};
    for (size_t i = 0; i < sizeof(argument); ++i)
        bytes.push_back(argument[i]);
    bytes.push_back(FVM_JIT_OPERAND);
    word(bytes, operand);
    bytes.push_back(FVM_JIT_ADDRESS_0);
    bytes.push_back(FVM_JIT_ADDRESS_1);
    unsigned char address[FVM_JIT_ADDRESS_BYTES];
    std::memcpy(address, &step, sizeof(step) < sizeof(address) ? sizeof(step) : sizeof(address));
    for (size_t i = 0; i < sizeof(address); ++i)
        bytes.push_back(address[i]);
    bytes.push_back(FVM_JIT_CALL_0);
    bytes.push_back(FVM_JIT_CALL_1);
}
/* Emit a status/predicate test and a rel32 conditional relocation. */
void conditional(fvm_detail::vector<unsigned char> &bytes, fvm_detail::vector<Fixup> &fixups,
                 size_t label)
{
    bytes.push_back(FVM_JIT_TEST_0);
    bytes.push_back(FVM_JIT_TEST_1);
    bytes.push_back(FVM_JIT_JNZ_0);
    bytes.push_back(FVM_JIT_JNZ_1);
    fixups.push_back(Fixup(bytes.size(), label));
    word(bytes, 0);
}
} // namespace
/* Preserve r12, maintain System V call alignment, and never unwind through executable pages. */
FVM_Result fvm_jit_emit(FVM_JitCode &code)
{
    fvm_detail::vector<unsigned char> bytes;
    fvm_detail::vector<Fixup> fixups;
    fvm_detail::vector<size_t> labels(code.actions.size());
    const unsigned char prologue[] = {FVM_JIT_ENDBR_0,    FVM_JIT_ENDBR_1,    FVM_JIT_ENDBR_2,
                                      FVM_JIT_ENDBR_3,    FVM_JIT_PUSH_R12_0, FVM_JIT_PUSH_R12_1,
                                      FVM_JIT_SAVE_ARG_0, FVM_JIT_SAVE_ARG_1, FVM_JIT_SAVE_ARG_2};
    for (size_t i = 0; i < sizeof(prologue); ++i)
        bytes.push_back(prologue[i]);
    for (size_t i = 0; i < code.actions.size(); ++i)
    {
        labels[i] = bytes.size();
        const FVM_JitAction &action = code.actions[i];
        for (size_t j = 0; j < action.calls.size(); ++j)
        {
            call(bytes, action.calls[j].step, action.calls[j].operand);
            conditional(bytes, fixups, FVM_JIT_EXIT_LABEL);
        }
        if (action.branch == FVM_JIT_CONDITIONAL)
        {
            call(bytes, fvm_jit_truth, 0);
            conditional(bytes, fixups, action.target);
        }
        else if (action.branch != FVM_JIT_FALLTHROUGH)
        {
            bytes.push_back(FVM_JIT_JMP);
            fixups.push_back(Fixup(bytes.size(), action.branch == FVM_JIT_EXIT ? FVM_JIT_EXIT_LABEL
                                                                               : action.target));
            word(bytes, 0);
        }
        if (bytes.size() > FVM_JIT_MAX_CODE_BYTES)
            return FVM_UNSUPPORTED;
    }
    size_t exit = bytes.size();
    bytes.push_back(FVM_JIT_POP_R12_0);
    bytes.push_back(FVM_JIT_POP_R12_1);
    bytes.push_back(FVM_JIT_RET);
    for (size_t i = 0; i < fixups.size(); ++i)
    {
        const Fixup &fixup = fixups[i];
        size_t target = fixup.label == FVM_JIT_EXIT_LABEL ? exit : labels[fixup.label];
        unsigned long relative = (unsigned long)(target - (fixup.at + FVM_JIT_IMMEDIATE_BYTES));
        for (unsigned int j = 0; j < FVM_JIT_IMMEDIATE_BYTES; ++j)
            bytes[fixup.at + j] =
                (unsigned char)((relative >> (j * FVM_JIT_BYTE_BITS)) & FVM_JIT_BYTE_MASK);
    }
    FVM_Result status =
        fvm_port_code_create(bytes.begin(), bytes.size(), &code.memory, &code.mapped);
    if (status == FVM_OK)
        code.bytes = bytes.size();
    return status;
}
