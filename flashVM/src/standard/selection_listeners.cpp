/* FlashVM: Selection focus observers, independent from Key listener registrations. */
#include "selection_listeners.hpp"
#include "base_state.hpp"
#include "../core/execution_work_scope.hpp"
#include "../core/listener_work.hpp"
#include "../core/keyboard_state.hpp"
#include "../core/object_root_scope.hpp"
#include "flashvm/keyboard.h"

namespace
{
/* Match the MovieClip Selection profile rather than claiming SWF5 text-field support. */
bool supported(FVM_Context *context)
{
    return !context->swf_loaded || context->swf_info.version >= FVM_SELECTION_MIN_SWF_VERSION;
}
} // namespace

/* [GC:] Retain each listener graph once; ordinary invocation validates context ownership. */
FVM_Result fvm_selection_add_listener(FVM_Context *context, FVM_Object *, unsigned int count,
                                      const FVM_Value *args, FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    if (!fvm_keyboard_context_usable(context))
        return FVM_INVALID_ARGUMENT;
    if (!supported(context))
        return FVM_UNSUPPORTED;
    if (!count || args[0].type != FVM_VALUE_OBJECT || !args[0].object)
        return FVM_OK;
    return fvm_listener_add_budgeted(context, context->selection_listeners,
                                     args[0].object, FVM_SELECTION_MAX_LISTENERS);
}

/* Removal changes later transition snapshots, never an already prepared notification. */
FVM_Result fvm_selection_remove_listener(FVM_Context *context, FVM_Object *, unsigned int count,
                                         const FVM_Value *args, FVM_Value *out, void *)
{
    fvm_value_boolean(out, 0);
    if (!fvm_keyboard_context_usable(context))
        return FVM_INVALID_ARGUMENT;
    if (!supported(context))
        return FVM_UNSUPPORTED;
    if (!count || args[0].type != FVM_VALUE_OBJECT || !args[0].object)
        return FVM_OK;
    int removed = 0;
    FVM_Result result = fvm_listener_remove_budgeted(context, context->selection_listeners,
                                                    args[0].object, &removed);
    if (result == FVM_OK)
        fvm_value_boolean(out, removed);
    return result;
}

/* [GC:] Use native Array storage and root the result through every allocating element store. */
FVM_Result fvm_selection_listener_array(FVM_Context *context, FVM_Object *, unsigned int,
                                        const FVM_Value *, FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    if (!fvm_keyboard_context_usable(context))
        return FVM_INVALID_ARGUMENT;
    if (!supported(context))
        return FVM_UNSUPPORTED;
    FVM_ExecutionWorkScope work(context);
    FVM_Object *array = 0;
    FVM_Result result = fvm_array_create_like(
        context, 0, (unsigned long)context->selection_listeners.size(), &array, true);
    if (result != FVM_OK)
        return result;
    /* [GC:] Retain the unpublished snapshot without invoking a replaceable Array constructor. */
    FVM_ObjectRootScope root(context, array);
    for (size_t i = 0; i < context->selection_listeners.size(); ++i)
    {
        result = work.step();
        if (result != FVM_OK)
            return result;
        FVM_Value value;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = context->selection_listeners[i];
        result = fvm_array_store_own(context, array, (unsigned long)i, value);
        if (result != FVM_OK)
            return result;
    }
    out->type = FVM_VALUE_OBJECT;
    out->object = array;
    return FVM_OK;
}
