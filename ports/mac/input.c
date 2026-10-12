/* FlashVM: Portable platform adapter interface or implementation: input.
 * File: ports/mac/input.c.
 */

#include "../input_port.h"
#include <ApplicationServices/ApplicationServices.h>
/* Poll port mouse. */
int fvm_port_mouse_poll(FVM_PortMouseState *state)
{

    /* Initialization / Инициализация локального состояния. */
    CGEventRef event;
    CGPoint p;
    if (!state)
        return 0;
    event = CGEventCreate(NULL);
    if (!event)
        return 0;

    /* Computation / Вычисление и обновление состояния. */
    p = CGEventGetLocation(event);
    state->x = (long)p.x;
    state->y = (long)p.y;
    state->buttons =
        (CGEventSourceButtonState(kCGEventSourceStateCombinedSessionState, kCGMouseButtonLeft)
             ? 1U
             : 0U) |
        (CGEventSourceButtonState(kCGEventSourceStateCombinedSessionState, kCGMouseButtonRight)
             ? 2U
             : 0U) |
        (CGEventSourceButtonState(kCGEventSourceStateCombinedSessionState, kCGMouseButtonCenter)
             ? 4U
             : 0U);
    CFRelease(event);
    return 1;
}

/* Poll port android touch. */
int fvm_port_android_touch_poll(void *queue, unsigned int *fingers, float *x, float *y, int *action)
{

    /* Computation / Вычисление и обновление состояния. */
    (void)queue;
    (void)fingers;
    (void)x;
    (void)y;
    (void)action;
    return -1;
}
