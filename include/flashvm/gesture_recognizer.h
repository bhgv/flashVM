#ifndef FLASHVM_GESTURE_RECOGNIZER_H
#define FLASHVM_GESTURE_RECOGNIZER_H
#include "input.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Host supplies monotonic milliseconds and centroid coordinates in stage pixels.
 * Finger count is the number of simultaneously active contacts. One gesture
 * sequence must begin with BEGIN and finish with END or CANCEL. */
typedef enum FVM_TouchAction {
    FVM_TOUCH_BEGIN=1, FVM_TOUCH_MOVE=2, FVM_TOUCH_END=3, FVM_TOUCH_CANCEL=4
} FVM_TouchAction;
typedef struct FVM_GestureRecognizer {
    unsigned long struct_size;
    double start_x, start_y;
    unsigned long start_ms;
    unsigned int max_fingers;
    double tap_max_distance;
    double swipe_min_distance;
    unsigned long tap_max_duration_ms;
    int active;
} FVM_GestureRecognizer;
FVM_API FVM_Result fvm_gesture_recognizer_init(FVM_GestureRecognizer *recognizer);
/* Returns FVM_OK even if the gesture is unrecognized; dispatched is 1 only
 * when a recognized gesture was sent to the existing input callback. */
FVM_API FVM_Result fvm_gesture_recognizer_feed(
    FVM_Context *context, FVM_GestureRecognizer *recognizer,
    FVM_TouchAction action, unsigned int fingers,
    double x, double y, unsigned long milliseconds, int *dispatched);
#ifdef __cplusplus
}
#endif
#endif
