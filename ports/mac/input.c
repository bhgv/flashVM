#include "../input_port.h"
#include <ApplicationServices/ApplicationServices.h>
int fvm_port_mouse_poll(FVM_PortMouseState *state) {
    CGEventRef event;
    CGPoint p;
    if (!state) return 0;
    event = CGEventCreate(NULL);
    if (!event) return 0;
    p = CGEventGetLocation(event);
    state->x = (long)p.x; state->y = (long)p.y;
    state->buttons = (CGEventSourceButtonState(kCGEventSourceStateCombinedSessionState, kCGMouseButtonLeft) ? 1U : 0U)
        | (CGEventSourceButtonState(kCGEventSourceStateCombinedSessionState, kCGMouseButtonRight) ? 2U : 0U)
        | (CGEventSourceButtonState(kCGEventSourceStateCombinedSessionState, kCGMouseButtonCenter) ? 4U : 0U);
    CFRelease(event);
    return 1;
}

int fvm_port_android_touch_poll(void *queue, unsigned int *fingers,
                                float *x, float *y, int *action) {
    (void)queue; (void)fingers; (void)x; (void)y; (void)action;
    return -1;
}
