/* FlashVM: strongly rooted Mouse registrations and mutation-safe synchronous notification. */
#include "../core/mouse_state.hpp"
#include "../core/optional_method.hpp"
#include "../core/mouse_limits.h"
#include "../core/value_vector_root_scope.hpp"
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
        context_->mouse_dispatching = true;
    }
    /* Restore ordinary dispatch, including after handler exceptions. */
    ~DeliveryScope()
    {
        context_->mouse_dispatching = false;
    }
};
} // namespace

/* [GC:] Retain identity once, independently of the current Mouse global binding. */
extern "C" FVM_Result fvm_input_add_mouse_listener(FVM_Context *context, FVM_Object *listener)
{
    if (!fvm_mouse_context_usable(context) || !listener || listener->owner != context)
        return FVM_INVALID_ARGUMENT;
    return fvm_listener_add_budgeted(context, context->mouse_listeners, listener, FVM_MOUSE_MAX_LISTENERS);
}

/* Remove a current registration without allocating; an active delivery retains its own snapshot. */
extern "C" FVM_Result fvm_input_remove_mouse_listener(FVM_Context *context, FVM_Object *listener,
                                                    int *removed)
{
    if (!fvm_mouse_context_usable(context) || !listener || listener->owner != context ||
        !removed)
        return FVM_INVALID_ARGUMENT;
    return fvm_listener_remove_budgeted(context, context->mouse_listeners, listener, removed);
}

namespace
{
/* [GC:] Prepare a strong receiver snapshot before publishing coordinates; arguments are native Numbers. */
FVM_Result dispatch(FVM_Context *context, const char *handler, double x, double y,
                    unsigned int argc, const FVM_Value *args)
{
    try
    {
        FVM_ExecutionWorkScope work(context);
        fvm_detail::vector<FVM_Value> recipients;
        /* [GC:] Pending receivers survive unsubscription and collection inside earlier handlers. */
        FVM_ValueVectorRootScope roots(context, recipients);
        recipients.reserve(context->mouse_listeners.size());
        for (size_t i = 0; i < context->mouse_listeners.size(); ++i)
        {
            FVM_Result charged = work.step();
            if (charged != FVM_OK)
                return charged;
            FVM_Value value;
            fvm_value_undefined(&value);
            value.type = FVM_VALUE_OBJECT;
            value.object = context->mouse_listeners[i];
            recipients.push_back(value);
        }
        FVM_Result result = work.step();
        if (result != FVM_OK)
            return result;
        DeliveryScope delivery(context);
        result = fvm_input_set_pointer(context, x, y);
        if (result == FVM_OK)
            result = fvm_exec_drag_update(context);
        if (result != FVM_OK)
            return result;
        for (size_t i = 0; i < recipients.size(); ++i)
        {
            result = work.step();
            if (result != FVM_OK)
                return result;
            result = fvm_object_call_optional_args(context, recipients[i].object, handler, argc, args);
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

} // namespace

/* Validate a primary-pointer transition before shared rooted dispatch can change state. */
extern "C" FVM_Result fvm_input_dispatch_mouse(FVM_Context *context, FVM_MouseEventType type,
                                               double x, double y)
{
    if (!fvm_mouse_context_usable(context) || !fvm_mouse_event_valid(type, x, y) ||
        context->mouse_dispatching)
        return FVM_INVALID_ARGUMENT;
    const char *handler = type == FVM_MOUSE_EVENT_MOVE ? "onMouseMove"
                          : type == FVM_MOUSE_EVENT_DOWN ? "onMouseDown" : "onMouseUp";
    return dispatch(context, handler, x, y, 0, 0);
}

/* Preserve a signed finite line delta as a Number; hit testing and platform normalization belong to host. */
extern "C" FVM_Result fvm_input_dispatch_mouse_wheel(FVM_Context *context, double delta,
                                                     double x, double y)
{
    if (!fvm_mouse_context_usable(context) || !fvm_mouse_wheel_valid(delta, x, y) ||
        context->mouse_dispatching)
        return FVM_INVALID_ARGUMENT;
    FVM_Value value;
    fvm_value_number(&value, delta);
    return dispatch(context, "onMouseWheel", x, y, FVM_MOUSE_WHEEL_DELTA_ARGS, &value);
}
