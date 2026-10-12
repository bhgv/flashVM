/* FlashVM: Portable platform adapter interface or implementation: input.
 * File: ports/android/input.c.
 */

#include "../input_port.h"
#include <android/input.h>
/* Android has no global cursor. The owning Activity must supply an
 * AInputQueue or dispatch AMotionEvent to the host; global polling is invalid.
 * This port intentionally reports unavailable rather than fake coordinates. */
/* Poll port mouse. */
int fvm_port_mouse_poll(FVM_PortMouseState *state)
{
    (void)state;
    return 0;
}

/* Poll port android touch. */
int fvm_port_android_touch_poll(void *queue, unsigned int *fingers, float *x, float *y, int *action)
{

    /* Initialization / Инициализация локального состояния. */
    AInputEvent *event = 0;
    int status;
    if (!queue || !fingers || !x || !y || !action)
        return -1;
    status = AInputQueue_getEvent((AInputQueue *)queue, &event);
    if (status < 0)
        return 0;
    if (AInputQueue_preDispatchEvent((AInputQueue *)queue, event))
        return 0;
    if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION)
    {
        AInputQueue_finishEvent((AInputQueue *)queue, event, 0);
        return 0;
    }

    /* Computation / Вычисление и обновление состояния. */
    *fingers = (unsigned int)AMotionEvent_getPointerCount(event);
    *x = AMotionEvent_getX(event, 0);
    *y = AMotionEvent_getY(event, 0);
    *action = AMotionEvent_getAction(event);
    AInputQueue_finishEvent((AInputQueue *)queue, event, 1);
    return 1;
}
