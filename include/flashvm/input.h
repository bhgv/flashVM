/* FlashVM: Public embedding API declarations: input.
 * File: include/flashvm/input.h.
 */

#ifndef FLASHVM_INPUT_H
/* Define FLASHVM INPUT H constant or compilation switch. */
#define FLASHVM_INPUT_H
#include "flashvm.h"
#include "input_limits.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* A gesture is recognized by the host. FlashVM maps it to a synthetic mouse event. */
    /* Define FVM_Gesture enum and its representation. */
    typedef enum FVM_Gesture
    {
        FVM_GESTURE_TAP = 1 /* Recognized gesture: tap. */,
        FVM_GESTURE_SWIPE = 2 /* Recognized gesture: swipe. */,
        FVM_GESTURE_WHEEL = 3 /* Recognized gesture: wheel. */
    } FVM_Gesture;
    /* Define FVM_MouseButton enum and its representation. */
    typedef enum FVM_MouseButton
    {
        FVM_MOUSE_NONE = 0 /* Mouse button or wheel state: none. */,
        FVM_MOUSE_LEFT = 1 /* Mouse button or wheel state: left. */,
        FVM_MOUSE_RIGHT = 2 /* Mouse button or wheel state: right. */,
        FVM_MOUSE_MIDDLE = 3 /* Mouse button or wheel state: middle. */,
        FVM_MOUSE_WHEEL_UP = 4 /* Mouse button or wheel state: wheel up. */,
        FVM_MOUSE_WHEEL_DOWN = 5 /* Mouse button or wheel state: wheel down. */
    } FVM_MouseButton;
    /* Define FVM_InputRule struct and its representation. */
    typedef struct FVM_InputRule
    {
        unsigned long struct_size; /* Size of this structure for API compatibility checks. */
        FVM_Gesture gesture;       /* Recognized gesture kind. */
        unsigned int fingers;      /* Number of fingers participating in the gesture. */
        int direction;             /* Direction of text or a recognized gesture. */
        FVM_MouseButton button;    /* Mouse button identifier. */
    } FVM_InputRule;
    /* Define FVM_InputEvent struct and its representation. */
    typedef struct FVM_InputEvent
    {
        FVM_Gesture gesture;  /* Recognized gesture kind. */
        unsigned int fingers; /* Finger count: 1 through FVM_INPUT_MAX_FINGERS. */
        int direction;        /* Direction of text or a recognized gesture. */
        double x;             /* Horizontal coordinate. */
        double y;             /* Vertical coordinate. */
    } FVM_InputEvent;
    /* direction: -1=left, 1=right, -2=up, 2=down, 0=any. */
    /* Update input rule. */
    FVM_API FVM_Result fvm_input_set_rule(FVM_Context *, const FVM_InputRule *);
    /* Implement input map for this subsystem. */
    FVM_API FVM_Result fvm_input_map(FVM_Context *, const FVM_InputEvent *, FVM_MouseButton *);
    /* Host-driven input notifications; callback runs synchronously during dispatch.
     * A gesture is always reported, even if no mouse mapping exists.
     * Callback must not destroy its context; pointers are valid only during call.
     */
    /* Define FVM_InputPhase enum and its representation. */
    typedef enum FVM_InputPhase
    {
        FVM_INPUT_BEGIN = 1 /* Input phase: begin. */,
        FVM_INPUT_UPDATE = 2 /* Input phase: update. */,
        FVM_INPUT_END = 3 /* Input phase: end. */
    } FVM_InputPhase;
    typedef void (*FVM_InputCallback)(FVM_Context *, const FVM_InputEvent *, FVM_InputPhase,
                                      FVM_MouseButton, void *);
    /* Update input callback. */
    FVM_API FVM_Result fvm_input_set_callback(FVM_Context *, FVM_InputCallback, void *);
    /* Dispatch a finite-coordinate event with supported gesture, finger count and phase.
     * Invalid input invokes no callback. Direct mapping preserves output on invalid input. */
    FVM_API FVM_Result fvm_input_dispatch(FVM_Context *, const FVM_InputEvent *, FVM_InputPhase);

#ifdef __cplusplus
}
#endif
#endif
