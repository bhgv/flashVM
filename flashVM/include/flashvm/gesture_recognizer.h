/* FlashVM: Public embedding API declarations: gesture recognizer.
 * File: include/flashvm/gesture_recognizer.h.
 */

#ifndef FLASHVM_GESTURE_RECOGNIZER_H
/* Define FLASHVM GESTURE RECOGNIZER H constant or compilation switch. */
#define FLASHVM_GESTURE_RECOGNIZER_H
#include "input.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Host supplies monotonic milliseconds and centroid coordinates in stage pixels.
     * Finger count is the number of simultaneously active contacts. One gesture
     * sequence must begin with BEGIN and finish with END or CANCEL. */
    /* Define FVM_TouchAction enum and its representation. */
    typedef enum FVM_TouchAction
    {
        FVM_TOUCH_BEGIN = 1 /* Touch phase: begin. */,
        FVM_TOUCH_MOVE = 2 /* Touch phase: move. */,
        FVM_TOUCH_END = 3 /* Touch phase: end. */,
        FVM_TOUCH_CANCEL = 4 /* Touch phase: cancel. */
    } FVM_TouchAction;
    /* Define FVM_GestureRecognizer struct and its representation. */
    typedef struct FVM_GestureRecognizer
    {
        unsigned long struct_size; /* Size of this structure for API compatibility checks. */
        double start_x /* Horizontal coordinate at gesture start. */,
            start_y;                       /* Vertical coordinate at gesture start. */
        unsigned long start_ms;            /* Timestamp of gesture start in milliseconds. */
        unsigned int max_fingers;          /* Maximum simultaneous touches tracked. */
        double tap_max_distance;           /* Maximum movement allowed for a tap. */
        double swipe_min_distance;         /* Minimum distance required to recognize a swipe. */
        unsigned long tap_max_duration_ms; /* Maximum tap duration in milliseconds. */
        int active;                        /* Whether the gesture is currently active. */
    } FVM_GestureRecognizer;
    /* Initialize gesture recognizer. */
    FVM_API FVM_Result fvm_gesture_recognizer_init(FVM_GestureRecognizer *recognizer);
    /* Returns FVM_OK even if the gesture is unrecognized; dispatched is 1 only
     * when a recognized gesture was sent to the existing input callback. */
    /* Implement gesture recognizer feed for this subsystem. */
    FVM_API FVM_Result fvm_gesture_recognizer_feed(FVM_Context *context,
                                                   FVM_GestureRecognizer *recognizer,
                                                   FVM_TouchAction action, unsigned int fingers,
                                                   double x, double y, unsigned long milliseconds,
                                                   int *dispatched);
#ifdef __cplusplus
}
#endif
#endif
