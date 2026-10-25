/* FlashVM: nonmutating keyboard validation shared by polling publication and notifications. */
#ifndef FVM_CORE_KEYBOARD_STATE_HPP
#define FVM_CORE_KEYBOARD_STATE_HPP /* Guard the private snapshot validation helpers. */
#include "runtime.hpp"


/* Reject teardown/collector reentry before reading or publishing host input. */
inline bool fvm_keyboard_context_usable(FVM_Context *context)
{
    return context && !context->destroying && !context->gc_collecting && !context->gc_releasing;
}


/* Check all native scalar fields without retaining pointers or modifying old state. */
inline bool fvm_keyboard_state_valid(const FVM_KeyboardState *state)
{
    if (!state || state->struct_size != sizeof(*state) || state->down[0] ||
        state->last_code >= FVM_KEYBOARD_CODE_COUNT || state->last_ascii > FVM_KEYBOARD_ASCII_MAX ||
        (state->caps_lock != 0 && state->caps_lock != 1) || (state->num_lock != 0 && state->num_lock != 1))
    {
        return false;
    }
    for (size_t code = 0; code < FVM_KEYBOARD_CODE_COUNT; ++code)
    {
        if (state->down[code] > 1)
        {
            return false;
        }
    }
    return true;
}
#endif
