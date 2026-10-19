/* FlashVM: strongly rooted keyboard registrations and mutation-safe synchronous notification. */
#include "../core/keyboard_state.hpp"
#include "../core/optional_method.hpp"
#include "../core/value_vector_root_scope.hpp"
#include "../standard/movieclip_scene.hpp"
#include "../standard/movieclip_state.hpp"
#include "../core/listener_work.hpp"
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
    return fvm_listener_add_budgeted(context, context->keyboard_listeners, listener, FVM_KEYBOARD_MAX_LISTENERS);
}

/* Remove a current registration without allocating; an active delivery retains its own snapshot. */
extern "C" FVM_Result fvm_input_remove_key_listener(FVM_Context *context, FVM_Object *listener,
                                                    int *removed)
{
    if (!fvm_keyboard_context_usable(context) || !listener || listener->owner != context ||
        !removed)
        return FVM_INVALID_ARGUMENT;
    return fvm_listener_remove_budgeted(context, context->keyboard_listeners, listener, removed);
}

/* Select only native instances; private attachment checks never invoke script getters. */
extern "C" FVM_Result fvm_input_set_keyboard_focus(FVM_Context *context, FVM_Object *clip)
{
    try
    {
        if (!fvm_keyboard_context_usable(context))
            return FVM_INVALID_ARGUMENT;
        if (clip)
        {
            if (clip->owner != context || clip->resource_kind != FVM_RESOURCE_MOVIECLIP ||
                fvm_movieclip_is_class_prototype(clip))
                return FVM_INVALID_ARGUMENT;
            bool removed = false;
            FVM_Result result = fvm_movieclip_scene_removed(context, clip, &removed);
            if (result != FVM_OK)
                return result;
            if (removed)
                return FVM_INVALID_ARGUMENT;
        }
        /* [GC:] Replacement publishes the new root without any allocating work afterwards. */
        context->keyboard_focus = clip;
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

/* Lazily release detached selections, retaining the previous output if validation fails. */
extern "C" FVM_Result fvm_input_get_keyboard_focus(FVM_Context *context, FVM_Object **out)
{
    try
    {
        if (!fvm_keyboard_context_usable(context) || !out)
            return FVM_INVALID_ARGUMENT;
        if (context->keyboard_focus)
        {
            bool removed = false;
            FVM_Result result =
                fvm_movieclip_scene_removed(context, context->keyboard_focus, &removed);
            if (result != FVM_OK)
                return result;
            if (removed)
                context->keyboard_focus = 0; /* [GC:] Drop the detached graph's selection root. */
        }
        *out = context->keyboard_focus;
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

/* Prepare all recipients before publication, then resolve each current handler exactly once. */
extern "C" FVM_Result fvm_input_dispatch_keyboard(FVM_Context *context,
                                                  const FVM_KeyboardState *state, int pressed)
{
    if (!fvm_keyboard_context_usable(context) || !fvm_keyboard_state_valid(state) ||
        (pressed != 0 && pressed != 1) || context->keyboard_dispatching)
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_ExecutionWorkScope work(context);
        fvm_detail::vector<FVM_Value> recipients;
        /* [GC:] Link the snapshot before any handler can remove a later listener and collect. */
        FVM_ValueVectorRootScope roots(context, recipients);
        FVM_Object *focus = 0;
        FVM_Result status = fvm_input_get_keyboard_focus(context, &focus);
        if (status != FVM_OK)
            return status;
        recipients.reserve(context->keyboard_listeners.size() + (focus ? 1 : 0));
        if (focus)
        {
            status = work.step();
            if (status != FVM_OK)
                return status;
            FVM_Value value;
            fvm_value_undefined(&value);
            value.type = FVM_VALUE_OBJECT;
            value.object = focus;
            recipients.push_back(value);
        }
        for (size_t i = 0; i < context->keyboard_listeners.size(); ++i)
        {
            FVM_Result charged = work.step();
            if (charged != FVM_OK)
                return charged;
            FVM_Value value;
            fvm_value_undefined(&value);
            value.type = FVM_VALUE_OBJECT;
            value.object = context->keyboard_listeners[i];
            recipients.push_back(value);
        }
        status = work.step();
        if (status != FVM_OK)
            return status;
        context->keyboard = *state;
        DeliveryScope delivery(context);
        for (size_t i = 0; i < recipients.size(); ++i)
        {
            FVM_Result result = work.step();
            if (result != FVM_OK)
                return result;
            result = fvm_object_call_optional(context, recipients[i].object,
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
