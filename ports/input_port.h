#ifndef FVM_INPUT_PORT_H
#define FVM_INPUT_PORT_H
#ifdef __cplusplus
extern "C" {
#endif
/* Physical mouse snapshot, in platform screen coordinates. */
typedef struct FVM_PortMouseState {
    long x, y;
    unsigned int buttons; /* bit 0 left, bit 1 right, bit 2 middle */
} FVM_PortMouseState;
/* Returns 1 when supported and sampled, 0 otherwise. */
int fvm_port_mouse_poll(FVM_PortMouseState *state);
/* Android only: poll a NativeActivity AInputQueue pointer. Returns 1 on
 * motion, 0 if no event, -1 on error. Touch pointer count is returned. */
int fvm_port_android_touch_poll(void *input_queue, unsigned int *fingers,
                                float *x, float *y, int *action);
#ifdef __cplusplus
}
#endif
#endif
