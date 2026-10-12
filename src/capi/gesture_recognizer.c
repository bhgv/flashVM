/* FlashVM: C embedding API: gesture recognizer.
 * File: src/capi/gesture_recognizer.c.
 */

#include "flashvm/gesture_recognizer.h"
#include <math.h>
#include <stddef.h>
/* Initialize gesture recognizer. */
FVM_Result fvm_gesture_recognizer_init(FVM_GestureRecognizer *r)
{
    if (!r || r->struct_size != sizeof(*r))
        return FVM_INVALID_ARGUMENT;

    /* Computation / Вычисление и обновление состояния. */
    r->start_x = 0;
    r->start_y = 0;
    r->start_ms = 0;
    r->max_fingers = 0;
    r->tap_max_distance = 12.0;
    r->swipe_min_distance = 48.0;
    r->tap_max_duration_ms = 350UL;
    r->active = 0;
    return FVM_OK;
}

/* Compute and return gesture recognizer feed. */
FVM_Result fvm_gesture_recognizer_feed(FVM_Context *ctx, FVM_GestureRecognizer *r,
                                       FVM_TouchAction action, unsigned int fingers, double x,
                                       double y, unsigned long ms, int *dispatched)
{

    /* Initialization / Инициализация локального состояния. */
    double dx, dy, distance;
    unsigned long elapsed;
    FVM_InputEvent event;
    if (!ctx || !r || r->struct_size != sizeof(*r) || !dispatched || fingers > 32U || x != x ||
        y != y || fabs(x) > 1.0e100 || fabs(y) > 1.0e100)
        return FVM_INVALID_ARGUMENT;
    *dispatched = 0;
    if (action == FVM_TOUCH_BEGIN)
    {
        if (r->active || fingers == 0U)
            return FVM_INVALID_ARGUMENT;
        r->start_x = x;
        r->start_y = y;
        r->start_ms = ms;
        r->max_fingers = fingers;
        r->active = 1;
        return FVM_OK;
    }
    if (action == FVM_TOUCH_CANCEL)
    {
        r->active = 0;
        return FVM_OK;
    }
    if (!r->active || ms < r->start_ms)
        return FVM_INVALID_ARGUMENT;
    if (fingers > r->max_fingers)
        r->max_fingers = fingers;
    if (action == FVM_TOUCH_MOVE)
        return FVM_OK;
    if (action != FVM_TOUCH_END)
        return FVM_INVALID_ARGUMENT;

    /* Computation / Вычисление и обновление состояния. */
    r->active = 0;
    dx = x - r->start_x;
    dy = y - r->start_y;
    distance = sqrt(dx * dx + dy * dy);
    elapsed = ms - r->start_ms;
    event.fingers = r->max_fingers;
    event.x = x;
    event.y = y;
    event.direction = 0;
    if (distance <= r->tap_max_distance && elapsed <= r->tap_max_duration_ms)
        event.gesture = FVM_GESTURE_TAP;
    else if (distance >= r->swipe_min_distance)
    {
        event.gesture = FVM_GESTURE_SWIPE;
        event.direction = (fabs(dx) >= fabs(dy)) ? (dx < 0 ? -1 : 1) : (dy < 0 ? -2 : 2);
    }
    else
        return FVM_OK;
    if (fvm_input_dispatch(ctx, &event, FVM_INPUT_END) != FVM_OK)
        return FVM_ERROR;
    *dispatched = 1;
    return FVM_OK;
}
