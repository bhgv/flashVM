/* FlashVM: common call-plan lowering and checked native branch relocations. */
#include "machine.hpp"
/* Serialize instructions without depending on host endian or long width. */
void fvm_jit_word(fvm_detail::vector<unsigned char> &b, unsigned long value)
{
    for (size_t i = 0; i < FVM_JIT_IMMEDIATE_BYTES; ++i)
        b.push_back((unsigned char)((value >> (i * FVM_JIT_BYTE_BITS)) & FVM_JIT_BYTE_MASK));
}
namespace
{
/* A patch names the last instruction/immediate emitted by a branch operation. */
struct Patch
{
    size_t at;    /* Branch displacement or final instruction byte offset. */
    size_t label; /* Action index or common epilogue sentinel. */
    /* Construct a relocation, including unused vector capacity slots. */
    Patch(size_t a = 0, size_t l = 0) : at(a), label(l)
    {
    }
};
/* Route one machine operation to a concrete encoder. */
void machine(FVM_JitTarget target, fvm_detail::vector<unsigned char> &b, FVM_JitMachineOp op,
             const unsigned char *address = 0, unsigned int operand = 0)
{
    switch (target)
    {
    case FVM_JIT_TARGET_X86:
        fvm_jit_machine_x86(b, op, address, operand);
        break;
    case FVM_JIT_TARGET_WINDOWS_X64:
        fvm_jit_machine_windows_x64(b, op, address, operand);
        break;
    case FVM_JIT_TARGET_MACOS_X64:
        fvm_jit_machine_macos_x64(b, op, address, operand);
        break;
    case FVM_JIT_TARGET_ARM:
        fvm_jit_machine_arm(b, op, address, operand);
        break;
    case FVM_JIT_TARGET_ARM64:
        fvm_jit_machine_arm64(b, op, address, operand);
        break;
    default:
        fvm_jit_machine_riscv(b, op, address, operand, target == FVM_JIT_TARGET_RISCV64);
        break;
    }
}
/* Replace a complete little-endian instruction/immediate. */
void patch(fvm_detail::vector<unsigned char> &b, size_t at, unsigned long value)
{
    for (size_t i = 0; i < FVM_JIT_IMMEDIATE_BYTES; ++i)
        b[at + i] = (unsigned char)((value >> (i * FVM_JIT_BYTE_BITS)) & FVM_JIT_BYTE_MASK);
}
/* Read one instruction before merging a bounded relocation field. */
unsigned long read(const fvm_detail::vector<unsigned char> &b, size_t at)
{
    unsigned long value = 0;
    for (size_t i = 0; i < FVM_JIT_IMMEDIATE_BYTES; ++i)
        value |= (unsigned long)b[at + i] << (i * FVM_JIT_BYTE_BITS);
    return value;
}
} // namespace
/* Addresses come from an explicit resolver; failed encoding leaves the caller's output unchanged.
 */
FVM_Result fvm_jit_encode(FVM_JitTarget target, const FVM_JitCode &code,
                          fvm_detail::vector<unsigned char> &output, FVM_JitResolver resolver,
                          void *userdata)
{
    if (((target < FVM_JIT_TARGET_X86 || target > FVM_JIT_TARGET_RISCV64) &&
         target != FVM_JIT_TARGET_WINDOWS_X64 && target != FVM_JIT_TARGET_MACOS_X64) || !resolver ||
        code.actions.empty())
        return FVM_UNSUPPORTED;
    fvm_detail::vector<unsigned char> b;
    fvm_detail::vector<Patch> patches;
    fvm_detail::vector<size_t> labels(code.actions.size());
    machine(target, b, FVM_JIT_MACHINE_ENTER);
    for (size_t i = 0; i < code.actions.size(); ++i)
    {
        labels[i] = b.size();
        const FVM_JitAction &action = code.actions[i];
        if (action.branch != FVM_JIT_FALLTHROUGH && action.branch != FVM_JIT_EXIT &&
            action.target >= code.actions.size())
            return FVM_INVALID_ARGUMENT;
        size_t calls = action.calls.size() + (action.branch == FVM_JIT_CONDITIONAL ? 1 : 0);
        for (size_t j = 0; j < calls; ++j)
        {
            bool predicate = j == action.calls.size();
            unsigned char address[FVM_JIT_POINTER64_BYTES] = {0};
            FVM_JitStep step = predicate ? fvm_jit_truth : action.calls[j].step;
            if (!resolver(step, address, userdata))
                return FVM_UNSUPPORTED;
            if (fvm_jit_target_width(target) == FVM_JIT_POINTER32_BYTES)
                for (size_t k = FVM_JIT_POINTER32_BYTES; k < sizeof(address); ++k)
                    if (address[k])
                        return FVM_UNSUPPORTED;
            machine(target, b, FVM_JIT_MACHINE_CALL, address,
                    predicate ? 0 : action.calls[j].operand);
            machine(target, b, FVM_JIT_MACHINE_NONZERO);
            patches.push_back(Patch(b.size() - FVM_JIT_IMMEDIATE_BYTES,
                                    predicate ? action.target : FVM_JIT_EXIT_LABEL));
            if (b.size() > FVM_JIT_MAX_CODE_BYTES)
                return FVM_UNSUPPORTED;
        }
        if (action.branch == FVM_JIT_JUMP || action.branch == FVM_JIT_EXIT)
        {
            machine(target, b, FVM_JIT_MACHINE_JUMP);
            patches.push_back(
                Patch(b.size() - FVM_JIT_IMMEDIATE_BYTES,
                      action.branch == FVM_JIT_EXIT ? FVM_JIT_EXIT_LABEL : action.target));
        }
        if (b.size() > FVM_JIT_MAX_CODE_BYTES)
            return FVM_UNSUPPORTED;
    }
    size_t exit = b.size();
    machine(target, b, FVM_JIT_MACHINE_LEAVE);
    if (b.size() > FVM_JIT_MAX_CODE_BYTES)
        return FVM_UNSUPPORTED;
    for (size_t i = 0; i < patches.size(); ++i)
    {
        const Patch &p = patches[i];
        size_t dest = p.label == FVM_JIT_EXIT_LABEL ? exit : labels[p.label];
        if (target == FVM_JIT_TARGET_X86 || target == FVM_JIT_TARGET_WINDOWS_X64 ||
            target == FVM_JIT_TARGET_MACOS_X64)
            patch(b, p.at, (unsigned long)(dest - p.at - FVM_JIT_IMMEDIATE_BYTES));
        else if (target == FVM_JIT_TARGET_ARM)
            patch(b, p.at,
                  read(b, p.at) | (((unsigned long)(dest - p.at - FVM_JIT_ARM_PC_BIAS) >>
                                    FVM_JIT_INSTRUCTION_SHIFT) &
                                   FVM_JIT_ARM_BRANCH_MASK));
        else if (target == FVM_JIT_TARGET_ARM64)
            patch(b, p.at,
                  read(b, p.at) | (((unsigned long)(dest - p.at) >> FVM_JIT_INSTRUCTION_SHIFT) &
                                   FVM_JIT_ARM64_BRANCH_MASK));
        else
        {
            size_t at = p.at - FVM_JIT_IMMEDIATE_BYTES;
            unsigned long delta = (unsigned long)(dest - at) & FVM_JIT_U32_MASK;
            patch(b, at, ((delta + FVM_JIT_RV_ROUND) & FVM_JIT_RV_HIGH_MASK) | FVM_JIT_RV_AUIPC_T0);
            patch(b, p.at,
                  ((delta & FVM_JIT_RV_LOW_MASK) << FVM_JIT_RV_IMMEDIATE_SHIFT) |
                      FVM_JIT_RV_JALR_T0);
        }
    }
    output = b;
    return FVM_OK;
}
