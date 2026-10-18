/* FlashVM: strongly rooted keyboard registrations and mutation-safe synchronous notification. */
#include "../core/keyboard_state.hpp"
#include "../core/optional_method.hpp"
#include "../core/value_vector_root_scope.hpp"
#include <new>

namespace
{
/* Restore recursive-dispatch protection on every return and C++ unwind path. */
class DeliveryScope
{
    FVM_Context *context_;                /* Borrowed until the dispatch scope is left. */
    DeliveryScope(const DeliveryScope &); /* A live delivery cannot be copied. */
    DeliveryScope &operator=(const DeliveryScope &); /* Nor can its guard be assigned. */
  public:
    /* Protect delivery before any script getter or callback can run. */
    explicit DeliveryScope(FVM_Context *context) : context_(context)
    {
        context_->keyboard_dispatching = true;
    }
    /* Restore ordinary dispatch, including after handler exceptions. */
    ~DeliveryScope()
    {
        context_->keyboard_dispatching = false;
    }
};
} // namespace

/* [GC:] Retain identity once, independently of the current Key global binding. */
extern "C" FVM_Result fvm_input_add_key_listener(FVM_Context *context, FVM_Object *listener)
{
    if (!fvm_keyboard_context_usable(context) || !listener || listener->owner != context)
        return FVM_INVALID_ARGUMENT;
    for (size_t i = 0; i < context->keyboard_listeners.size(); ++i)
        if (context->keyboard_listeners[i] == listener)
            return FVM_OK;
    if (context->keyboard_listeners.size() >= FVM_KEYBOARD_MAX_LISTENERS)
        return FVM_ERROR;
    try
    {
        context->keyboard_listeners.push_back(listener);
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
}

/* Remove a current registration without allocating; an active delivery retains its own snapshot. */
extern "C" FVM_Result fvm_input_remove_key_listener(FVM_Context *context, FVM_Object *listener,
                                                    int *removed)
{
    if (!fvm_keyboard_context_usable(context) || !listener || listener->owner != context ||
        !removed)
        return FVM_INVALID_ARGUMENT;
    for (size_t i = 0; i < context->keyboard_listeners.size(); ++i)
        if (context->keyboard_listeners[i] == listener)
        {
            context->keyboard_listeners.erase(context->keyboard_listeners.begin() + i);
            *removed = 1;
            return FVM_OK;
        }
    *removed = 0;
    return FVM_OK;
}

/* Prepare all recipients before publication, then resolve each current handler exactly once. */
extern "C" FVM_Result fvm_input_dispatch_keyboard(FVM_Context *context,
                                                  const FVM_KeyboardState *state, int pressed)
{
    if (!fvm_keyboard_context_usable(context) || !fvm_keyboard_state_valid(state) ||
        (pressed != 0 && pressed != 1) || context->keyboard_dispatching)
        return FVM_INVALID_ARGUMENT;
    try
    {
        fvm_detail::vector<FVM_Value> recipients;
        /* [GC:] Link the snapshot before any handler can remove a later listener and collect. */
        FVM_ValueVectorRootScope roots(context, recipients);
        recipients.reserve(context->keyboard_listeners.size());
        for (size_t i = 0; i < context->keyboard_listeners.size(); ++i)
        {
            FVM_Value value;
            fvm_value_undefined(&value);
            value.type = FVM_VALUE_OBJECT;
            value.object = context->keyboard_listeners[i];
            recipients.push_back(value);
        }
        context->keyboard = *state;
        DeliveryScope delivery(context);
        for (size_t i = 0; i < recipients.size(); ++i)
        {
            FVM_Result result = fvm_object_call_optional(context, recipients[i].object,
                                                         pressed ? "onKeyDown" : "onKeyUp");
            if (result != FVM_OK)
                return result;
        }
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
}
