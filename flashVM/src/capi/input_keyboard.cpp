/* FlashVM: context-local keyboard snapshots, published without script or allocation. */
#include "../core/runtime.hpp"
#include <cstring>

namespace
{
/* Reject teardown/collector reentry before touching host-visible native state. */
bool usable(FVM_Context *context)
{
    return context && !context->destroying && !context->gc_collecting && !context->gc_releasing;
}
} // namespace

/* Validate the complete candidate before committing any key or code. */
extern "C" FVM_Result fvm_input_set_keyboard(FVM_Context *context, const FVM_KeyboardState *state)
{
    if (!usable(context) || !state || state->struct_size != sizeof(*state) || state->down[0] ||
        state->last_code >= FVM_KEYBOARD_CODE_COUNT || state->last_ascii > FVM_KEYBOARD_ASCII_MAX ||
        (state->caps_lock != 0 && state->caps_lock != 1) ||
        (state->num_lock != 0 && state->num_lock != 1))
        return FVM_INVALID_ARGUMENT;
    for (size_t code = 0; code < FVM_KEYBOARD_CODE_COUNT; ++code)
        if (state->down[code] > 1)
            return FVM_INVALID_ARGUMENT;
    context->keyboard = *state;
    return FVM_OK;
}

/* The owning thread may inspect the state without invoking any script getter. */
extern "C" FVM_Result fvm_input_get_keyboard(FVM_Context *context, FVM_KeyboardState *state)
{
    if (!usable(context) || !state || state->struct_size != sizeof(*state))
        return FVM_INVALID_ARGUMENT;
    *state = context->keyboard;
    return FVM_OK;
}

/* Focus loss clears native input; it does not invent key-up listener events. */
extern "C" FVM_Result fvm_input_reset_keyboard(FVM_Context *context)
{
    if (!usable(context))
        return FVM_INVALID_ARGUMENT;
    std::memset(&context->keyboard, 0, sizeof(context->keyboard));
    context->keyboard.struct_size = sizeof(context->keyboard);
    return FVM_OK;
}
