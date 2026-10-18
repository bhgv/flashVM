/* FlashVM: one context-local random sequence for Math.random and execution services. */
#ifndef FVM_RANDOM_HPP
#define FVM_RANDOM_HPP /* Context random step guard. */
#include "runtime.hpp"
#include "numeric_constants.h"
#include "random_constants.h"
/* Advance the existing sequence modulo 2^32, independent of unsigned-long host width. */
inline unsigned long fvm_random_next(FVM_Context *context)
{
    context->random_state =
        (context->random_state * FVM_RANDOM_MULTIPLIER + FVM_RANDOM_INCREMENT) & FVM_UINT32_MASK;
    return context->random_state;
}
#endif
