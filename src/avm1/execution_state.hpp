/* FlashVM: private resumable interpreter block state; absent from the public C ABI. */
#ifndef FVM_EXECUTION_STATE_HPP
#define FVM_EXECUTION_STATE_HPP /* Private state guard. */
#include "../core/runtime.hpp"
/* Define Execution struct and its representation. */
struct Execution
{
    fvm_detail::vector<fvm_detail::string> constants; /* Captured constant pool. */
    fvm_detail::vector<FVM_Value> registers, stack;   /* [GC:] Rooted interpreter slots. */
    unsigned long steps;                              /* Counted executed instructions. */
    bool returned;                                    /* Whether a Return terminated the block. */
    bool target_initialized; /* Whether this execution has entered its initial display-target scope.
                              */
    /* Initialize Execution and its owned state. */
    Execution() : steps(0), returned(false), target_initialized(false)
    {
    }
};
#endif
