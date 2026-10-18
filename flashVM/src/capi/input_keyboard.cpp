/* FlashVM: context-local keyboard snapshots, published without script or allocation. */
#include "../core/keyboard_state.hpp"
#include <cstring>

/* Validate the complete candidate before committing any key or code. */
extern "C" FVM_Result fvm_input_set_keyboard(FVM_Context *context, const FVM_KeyboardState *state)
{
    if (!fvm_keyboard_context_usable(context) || !fvm_keyboard_state_valid(state))
        return FVM_INVALID_ARGUMENT;
    context->keyboard = *state;
    return FVM_OK;
}

/* The owning thread may inspect the state without invoking any script getter. */
extern "C" FVM_Result fvm_input_get_keyboard(FVM_Context *context, FVM_KeyboardState *state)
{
    if (!fvm_keyboard_context_usable(context) || !state || state->struct_size != sizeof(*state))
        return FVM_INVALID_ARGUMENT;
    *state = context->keyboard;
    return FVM_OK;
}

/* Focus loss clears native input; it does not invent key-up listener events. */
extern "C" FVM_Result fvm_input_reset_keyboard(FVM_Context *context)
{
    if (!fvm_keyboard_context_usable(context))
        return FVM_INVALID_ARGUMENT;
    std::memset(&context->keyboard, 0, sizeof(context->keyboard));
    context->keyboard.struct_size = sizeof(context->keyboard);
    return FVM_OK;
}
