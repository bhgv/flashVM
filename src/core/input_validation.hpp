/* FlashVM: common host event validation without mutation or callback execution. */
#ifndef FVM_INPUT_VALIDATION_HPP
#define FVM_INPUT_VALIDATION_HPP /* Shared host input validation guard. */
#include "flashvm/input.h"
#include "flashvm/input_limits.h"
#include <cfloat>
/* Require supported gesture/finger ranges and finite world coordinates before publishing input. */
inline bool fvm_input_event_valid(const FVM_InputEvent *event)
{
    return event && event->fingers && event->fingers <= FVM_INPUT_MAX_FINGERS &&
           event->gesture >= FVM_GESTURE_TAP && event->gesture <= FVM_GESTURE_WHEEL &&
           event->x >= -DBL_MAX && event->x <= DBL_MAX && event->y >= -DBL_MAX &&
           event->y <= DBL_MAX;
}
/* Reject unknown lifecycle phases consistently for immediate and queued events. */
inline bool fvm_input_phase_valid(FVM_InputPhase phase)
{
    return phase == FVM_INPUT_BEGIN || phase == FVM_INPUT_UPDATE || phase == FVM_INPUT_END;
}
#endif
