/* Owning-thread copied FIFO shared by gestures, Key and Mouse; independent of window SDKs. */
#include "../core/runtime.hpp"
#include "../core/input_validation.hpp"
#include "../core/keyboard_state.hpp"
#include "../core/mouse_state.hpp"
#include "../core/input_queue_limits.h"
#include <new>

namespace
{
/* Append atomically: invalid arguments/capacity/allocation failure retain the old FIFO. */
FVM_Result append(FVM_Context *context, const FVM_QueuedInput &item)
{
    if (context->queued_input.size() >= FVM_INPUT_MAX_PENDING_EVENTS)
        return FVM_ERROR;
    try { context->queued_input.push_back(item); }
    catch (const std::bad_alloc &) { return FVM_OUT_OF_MEMORY; }
    catch (...) { return FVM_ERROR; }
    return FVM_OK;
}
/* [GC:] Keep context lifetime protected through nested native/script callbacks and unwinding. */
class PumpScope
{
    FVM_Context *context_; /* Borrow owner until the entry batch completes. */
    PumpScope(const PumpScope &); /* An active guard cannot be copied. */
    PumpScope &operator=(const PumpScope &); /* Nor can its teardown protection be reassigned. */
public:
    /* Block recursive pumping and context destruction before the first event leaves the FIFO. */
    explicit PumpScope(FVM_Context *context) : context_(context) { context_->pumping_input = true; }
    /* Restore normal context lifecycle even when a dispatcher returns an error. */
    ~PumpScope() { context_->pumping_input = false; }
};
}

/* Copy a recognized gesture without entering the VM event dispatcher. */
extern "C" FVM_Result fvm_input_enqueue(FVM_Context *context, const FVM_InputEvent *event,
                                         FVM_InputPhase phase)
{
    if (!fvm_keyboard_context_usable(context) || !fvm_input_event_valid(event) ||
        !fvm_input_phase_valid(phase))
        return FVM_INVALID_ARGUMENT;
    FVM_QueuedInput item = FVM_QueuedInput();
    item.kind = FVM_QUEUED_GESTURE;
    item.event = *event;
    item.phase = phase;
    return append(context, item);
}

/* Copy a primary-pointer event in already converted world pixels. */
extern "C" FVM_Result fvm_input_enqueue_mouse(FVM_Context *context, FVM_MouseEventType type,
                                               double world_x, double world_y)
{
    if (!fvm_mouse_context_usable(context) || !fvm_mouse_event_valid(type, world_x, world_y))
        return FVM_INVALID_ARGUMENT;
    FVM_QueuedInput item = FVM_QueuedInput();
    item.kind = FVM_QUEUED_MOUSE;
    item.mouse_type = type;
    item.event.x = world_x;
    item.event.y = world_y;
    return append(context, item);
}

/* Capture normalized wheel Number and position atomically with the common bounded FIFO. */
extern "C" FVM_Result fvm_input_enqueue_mouse_wheel(FVM_Context *context, double delta,
                                                     double world_x, double world_y)
{
    if (!fvm_mouse_context_usable(context) || !fvm_mouse_wheel_valid(delta, world_x, world_y))
        return FVM_INVALID_ARGUMENT;
    FVM_QueuedInput item = FVM_QueuedInput();
    item.kind = FVM_QUEUED_MOUSE_WHEEL;
    item.wheel_delta = delta;
    item.event.x = world_x;
    item.event.y = world_y;
    return append(context, item);
}

/* Retain complete native key state and its pointer position by value, never by pointer. */
extern "C" FVM_Result fvm_input_enqueue_keyboard(FVM_Context *context,
                                                  const FVM_KeyboardState *state, int pressed,
                                                  double world_x, double world_y)
{
    if (!fvm_keyboard_context_usable(context) || !fvm_keyboard_state_valid(state) ||
        (pressed != 0 && pressed != 1) ||
        !fvm_mouse_event_valid(FVM_MOUSE_EVENT_MOVE, world_x, world_y))
        return FVM_INVALID_ARGUMENT;
    FVM_QueuedInput item = FVM_QueuedInput();
    item.kind = FVM_QUEUED_KEYBOARD;
    item.keyboard = *state;
    item.pressed = pressed;
    item.event.x = world_x;
    item.event.y = world_y;
    return append(context, item);
}

/* Cancel pending work and invalidate a callback's current entry batch without allocating. */
extern "C" FVM_Result fvm_input_clear_queue(FVM_Context *context)
{
    if (!fvm_keyboard_context_usable(context))
        return FVM_INVALID_ARGUMENT;
    context->queued_input.clear();
    ++context->input_queue_epoch;
    return FVM_OK;
}

/* Read the total count across all event variants, with no script or OS activity. */
extern "C" FVM_Result fvm_input_pending(FVM_Context *context, unsigned int *out)
{
    if (!fvm_keyboard_context_usable(context) || !out)
        return FVM_INVALID_ARGUMENT;
    *out = static_cast<unsigned int>(context->queued_input.size());
    return FVM_OK;
}

/* Deliver a bounded entry batch; callback additions cannot turn one tick into an endless loop. */
extern "C" FVM_Result fvm_input_pump(FVM_Context *context, unsigned int limit,
                                      unsigned int *processed)
{
    if (processed) *processed = 0;
    if (!fvm_keyboard_context_usable(context) || context->pumping_input)
        return FVM_INVALID_ARGUMENT;
    PumpScope guard(context);
    size_t remaining = context->queued_input.size();
    if (limit && remaining > limit) remaining = limit;
    const unsigned long epoch = context->input_queue_epoch;
    unsigned int count = 0;
    FVM_Result result = FVM_OK;
    while (remaining-- && !context->queued_input.empty() && epoch == context->input_queue_epoch)
    {
        const FVM_QueuedInput item = context->queued_input.front();
        context->queued_input.erase(context->queued_input.begin());
        if (item.kind == FVM_QUEUED_GESTURE)
            result = fvm_input_dispatch(context, &item.event, item.phase);
        else if (item.kind == FVM_QUEUED_MOUSE)
            result = fvm_input_dispatch_mouse(context, item.mouse_type, item.event.x, item.event.y);
        else if (item.kind == FVM_QUEUED_MOUSE_WHEEL)
            result = fvm_input_dispatch_mouse_wheel(context, item.wheel_delta, item.event.x, item.event.y);
        else
        {
            result = fvm_input_set_pointer(context, item.event.x, item.event.y);
            if (result == FVM_OK)
                result = fvm_input_dispatch_keyboard(context, &item.keyboard, item.pressed);
        }
        if (result != FVM_OK) break;
        ++count;
    }
    if (processed) *processed = count;
    return result;
}
