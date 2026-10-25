/* FlashVM: Portable platform adapter interface or implementation: input.
 * File: ports/windows/input.c.
 */

#include "../input_port.h"
#include <windows.h>


/* Poll port mouse. */
int fvm_port_mouse_poll(FVM_PortMouseState *state)
{
    POINT p;
    if (!state || !GetCursorPos(&p))
    {
        return 0;
    }

    /* Computation / Вычисление и обновление состояния. */
    state->x = p.x;
    state->y = p.y;
    state->buttons = ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) ? 1U : 0U) |
                     ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) ? 2U : 0U) |
                     ((GetAsyncKeyState(VK_MBUTTON) & 0x8000) ? 4U : 0U);
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
