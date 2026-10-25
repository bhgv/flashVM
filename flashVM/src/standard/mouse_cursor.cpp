/* FlashVM: script cursor methods change a context request, never a global OS cursor. */
#include "mouse_listeners.hpp"
#include "../core/mouse_state.hpp"

namespace
{


/* Ignore arguments without conversion; visibility transition itself allocates and calls nothing. */
FVM_Result cursor(FVM_Context *context, int visible, FVM_Value *out)
{
    fvm_value_undefined(out);
    if (!fvm_mouse_context_usable(context))
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (context->swf_loaded && context->swf_info.version < FVM_MOUSE_CURSOR_MIN_SWF_VERSION)
    {
        return FVM_UNSUPPORTED;
    }
    int previous = 0;
    FVM_Result result = fvm_input_set_cursor_visible(context, visible, &previous);
    if (result == FVM_OK)
    {
        fvm_value_number(out, previous);
    }
    return result;
}
}


/* Mouse.hide returns previous requested visibility; repeating hide returns zero. */
FVM_Result fvm_mouse_hide(FVM_Context *context, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *out, void *)
{
    return cursor(context, 0, out);
}


/* Mouse.show returns previous requested visibility; repeating show returns one. */
FVM_Result fvm_mouse_show(FVM_Context *context, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *out, void *)
{
    return cursor(context, 1, out);
}
