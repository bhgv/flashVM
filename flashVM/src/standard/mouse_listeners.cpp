/* FlashVM: Mouse listener registration methods and independent inspection snapshots. */
#include "mouse_listeners.hpp"
#include "base_state.hpp"
#include "../core/mouse_state.hpp"
#include "../core/object_root_scope.hpp"
#include "flashvm/mouse.h"

/* Script addListener returns Undefined; native API callers receive registration errors directly. */
FVM_Result fvm_mouse_add_listener(FVM_Context *context, FVM_Object *, unsigned int count,
                                const FVM_Value *args, FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    if (!fvm_mouse_context_usable(context))
        return FVM_INVALID_ARGUMENT;
    if (context->swf_loaded && context->swf_info.version < FVM_MOUSE_MIN_SWF_VERSION)
        return FVM_UNSUPPORTED;
    if (!count || args[0].type != FVM_VALUE_OBJECT || !args[0].object)
        return FVM_OK;
    return fvm_input_add_mouse_listener(context, args[0].object);
}

/* Script removeListener returns false for absent/non-object arguments without coercion. */
FVM_Result fvm_mouse_remove_listener(FVM_Context *context, FVM_Object *, unsigned int count,
                                   const FVM_Value *args, FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    if (!fvm_mouse_context_usable(context))
        return FVM_INVALID_ARGUMENT;
    if (context->swf_loaded && context->swf_info.version < FVM_MOUSE_MIN_SWF_VERSION)
        return FVM_UNSUPPORTED;
    int removed = 0;
    FVM_Result result = FVM_OK;
    if (count && args[0].type == FVM_VALUE_OBJECT && args[0].object)
        result = fvm_input_remove_mouse_listener(context, args[0].object, &removed);
    fvm_value_boolean(out, removed);
    return result;
}

/* [GC:] Build a detached Array using retained builtin layout; no script Array constructor/getter
 * runs. */
FVM_Result fvm_mouse_listener_array(FVM_Context *context, FVM_Object *, unsigned int,
                                  const FVM_Value *, FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    if (!fvm_mouse_context_usable(context))
        return FVM_INVALID_ARGUMENT;
    if (context->swf_loaded && context->swf_info.version < FVM_MOUSE_MIN_SWF_VERSION)
        return FVM_UNSUPPORTED;
    FVM_Object *array = 0;
    FVM_Result result = fvm_array_create_like(
        context, 0, (unsigned long)context->mouse_listeners.size(), &array, true);
    if (result != FVM_OK)
        return result;
    /* [GC:] Protect the unpublished result while preparing its copied object references. */
    FVM_ObjectRootScope root(context, array);
    for (size_t i = 0; i < context->mouse_listeners.size(); ++i)
    {
        FVM_Value value;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = context->mouse_listeners[i];
        result = fvm_array_store_own(context, array, (unsigned long)i, value);
        if (result != FVM_OK)
            return result;
    }
    fvm_value_undefined(out);
    out->type = FVM_VALUE_OBJECT;
    out->object = array;
    return FVM_OK;
}
