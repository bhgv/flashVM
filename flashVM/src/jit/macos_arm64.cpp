/* FlashVM: offline Darwin ARM64 integer calls with a linked frame and untouched X18.
 * This encoder does not select Apple Silicon execution or publish MAP_JIT memory.
 */
#include "machine.hpp"
#include "machine_constants.h"
#include "macos_arm64_constants.h"

/* Preserve Darwin's frame chain; reuse only the shared two-integer-argument lowering. */
void fvm_jit_machine_macos_arm64(fvm_detail::vector<unsigned char> &bytes, FVM_JitMachineOp op,
                                const unsigned char *address, unsigned int operand)
{
    if (op == FVM_JIT_MACHINE_ENTER)
    {
        fvm_jit_word(bytes, FVM_JIT_ARM64_BTI_C);
        fvm_jit_word(bytes, FVM_JIT_MAC_A64_SAVE_STATE);
        fvm_jit_word(bytes, FVM_JIT_MAC_A64_SAVE_FRAME);
        fvm_jit_word(bytes, FVM_JIT_MAC_A64_LINK_FRAME);
        fvm_jit_word(bytes, FVM_JIT_ARM64_STATE_TO_X19);
    }
    else if (op == FVM_JIT_MACHINE_LEAVE)
    {
        fvm_jit_word(bytes, FVM_JIT_MAC_A64_RESTORE_FRAME);
        fvm_jit_word(bytes, FVM_JIT_MAC_A64_RESTORE_STATE);
        fvm_jit_word(bytes, FVM_JIT_ARM64_RET);
    }
    else
        fvm_jit_machine_arm64(bytes, op, address, operand);
}
