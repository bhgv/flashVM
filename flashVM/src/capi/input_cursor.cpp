/* FlashVM: allocation-free context-owned cursor request, independent of native window SDKs. */
#include "../core/mouse_state.hpp"

/* Return requested visibility, not an OS cursor/global display state. */
extern "C" FVM_Result fvm_input_get_cursor_visible(FVM_Context *context, int *visible)
{
    if (!fvm_mouse_context_usable(context) || !visible)
        return FVM_INVALID_ARGUMENT;
    *visible = context->cursor_visible ? 1 : 0;
    return FVM_OK;
}

/* Publish the logical request atomically on the owner thread and report the old logical state. */
extern "C" FVM_Result fvm_input_set_cursor_visible(FVM_Context *context, int visible, int *previous)
{
    if (!fvm_mouse_context_usable(context) || (visible != 0 && visible != 1) || !previous)
        return FVM_INVALID_ARGUMENT;
    const int before = context->cursor_visible ? 1 : 0;
    context->cursor_visible = visible != 0;
    *previous = before;
    return FVM_OK;
}
