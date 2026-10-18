/* FlashVM: validation shared by native Mouse registration, delivery and script methods. */
#ifndef FVM_CORE_MOUSE_STATE_HPP
#define FVM_CORE_MOUSE_STATE_HPP /* Guard the private owning-thread validation boundary. */
#include "runtime.hpp"
#include "flashvm/mouse.h"
#include <cfloat>
/* Reject collector/teardown entry before native registrations or script delivery change state. */
inline bool fvm_mouse_context_usable(FVM_Context *context)
{
    return context && !context->destroying && !context->gc_collecting && !context->gc_releasing;
}
/* Validate all event inputs before allocating, updating drag state or publishing coordinates. */
inline bool fvm_mouse_event_valid(FVM_MouseEventType type, double x, double y)
{
    return (type == FVM_MOUSE_EVENT_MOVE || type == FVM_MOUSE_EVENT_DOWN ||
            type == FVM_MOUSE_EVENT_UP) &&
           x >= -DBL_MAX && x <= DBL_MAX && y >= -DBL_MAX && y <= DBL_MAX;
}
#endif
