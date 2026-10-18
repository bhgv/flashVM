/* FlashVM: Portable platform adapter interface or implementation: input port.
 * File: ports/input_port.h.
 */

#ifndef FVM_INPUT_PORT_H
/* Define FVM INPUT PORT H constant or compilation switch. */
#define FVM_INPUT_PORT_H
#ifdef __cplusplus
extern "C"
{
#endif
    /* Physical mouse snapshot, in platform screen coordinates. */
    /* Define FVM_PortMouseState struct and its representation. */
    typedef struct FVM_PortMouseState
    {
        long x /* Horizontal coordinate. */, y;                  /* Vertical coordinate. */
        unsigned int buttons; /* Mouse button state bit mask. */ /* bit 0 left, bit 1 right,
                                                                                    bit 2 middle */
    } FVM_PortMouseState;
    /* Returns 1 when supported and sampled, 0 otherwise. */
    /* Poll port mouse. */
    int fvm_port_mouse_poll(FVM_PortMouseState *state);
    /* Android only: poll a NativeActivity AInputQueue pointer. Returns 1 on
     * motion, 0 if no event, -1 on error. Touch pointer count is returned. */
    /* Poll port android touch. */
    int fvm_port_android_touch_poll(void *input_queue, unsigned int *fingers, float *x, float *y,
                                    int *action);
#ifdef __cplusplus
}
#endif
#endif
