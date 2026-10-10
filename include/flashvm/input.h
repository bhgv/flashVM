#ifndef FLASHVM_INPUT_H
#define FLASHVM_INPUT_H
#include "flashvm.h"
#ifdef __cplusplus
extern "C" {
#endif
/* A gesture is recognized by the host. FlashVM maps it to a synthetic mouse event. */
typedef enum FVM_Gesture { FVM_GESTURE_TAP=1, FVM_GESTURE_SWIPE=2, FVM_GESTURE_WHEEL=3 } FVM_Gesture;
typedef enum FVM_MouseButton { FVM_MOUSE_NONE=0, FVM_MOUSE_LEFT=1, FVM_MOUSE_RIGHT=2, FVM_MOUSE_MIDDLE=3, FVM_MOUSE_WHEEL_UP=4, FVM_MOUSE_WHEEL_DOWN=5 } FVM_MouseButton;
typedef struct FVM_InputRule { unsigned long struct_size; FVM_Gesture gesture; unsigned int fingers; int direction; FVM_MouseButton button; } FVM_InputRule;
typedef struct FVM_InputEvent { FVM_Gesture gesture; unsigned int fingers; int direction; double x; double y; } FVM_InputEvent;
/* direction: -1=left, 1=right, -2=up, 2=down, 0=any. */
FVM_API FVM_Result fvm_input_set_rule(FVM_Context *, const FVM_InputRule *);
FVM_API FVM_Result fvm_input_map(FVM_Context *, const FVM_InputEvent *, FVM_MouseButton *);
#ifdef __cplusplus
}
#endif
#endif
