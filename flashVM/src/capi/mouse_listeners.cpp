/* FlashVM: strongly rooted Mouse registrations and mutation-safe synchronous notification. */
#include "../core/mouse_state.hpp"
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
    for (size_t i = 0; i < context->mouse_listeners.size(); ++i)
        if (context->mouse_listeners[i] == listener)
            return FVM_OK;
    if (context->mouse_listeners.size() >= FVM_MOUSE_MAX_LISTENERS)
        return FVM_ERROR;
    try
    {
        context->mouse_listeners.push_back(listener);
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
extern "C" FVM_Result fvm_input_remove_mouse_listener(FVM_Context *context, FVM_Object *listener,
                                                    int *removed)
{
    if (!fvm_mouse_context_usable(context) || !listener || listener->owner != context ||
        !removed)
        return FVM_INVALID_ARGUMENT;
    for (size_t i = 0; i < context->mouse_listeners.size(); ++i)
        if (context->mouse_listeners[i] == listener)
        {
            context->mouse_listeners.erase(context->mouse_listeners.begin() + i);
            *removed = 1;
            return FVM_OK;
        }
    *removed = 0;
    return FVM_OK;
}

/* Snapshot recipients before publication, then update drag state and deliver optional handlers. */
extern "C" FVM_Result fvm_input_dispatch_mouse(FVM_Context *context, FVM_MouseEventType type,
                                               double x, double y)
{
    if (!fvm_mouse_context_usable(context) || !fvm_mouse_event_valid(type, x, y) ||
        context->mouse_dispatching)
        return FVM_INVALID_ARGUMENT;
    try
    {
        fvm_detail::vector<FVM_Value> recipients;
        /* [GC:] Pending receivers survive unsubscription and collection inside earlier handlers. */
        FVM_ValueVectorRootScope roots(context, recipients);
        recipients.reserve(context->mouse_listeners.size());
        for (size_t i = 0; i < context->mouse_listeners.size(); ++i)
        {
            FVM_Value value;
            fvm_value_undefined(&value);
            value.type = FVM_VALUE_OBJECT;
            value.object = context->mouse_listeners[i];
            recipients.push_back(value);
        }
        DeliveryScope delivery(context);
        FVM_Result result = fvm_input_set_pointer(context, x, y);
        if (result == FVM_OK)
            result = fvm_exec_drag_update(context);
        if (result != FVM_OK)
            return result;
        const char *handler = type == FVM_MOUSE_EVENT_MOVE ? "onMouseMove"
                              : type == FVM_MOUSE_EVENT_DOWN ? "onMouseDown" : "onMouseUp";
        for (size_t i = 0; i < recipients.size(); ++i)
        {
            result = fvm_object_call_optional(context, recipients[i].object, handler);
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
