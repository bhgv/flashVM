/* C89 example: host delivers touch samples to FlashVM. */
#include "flashvm/gesture_recognizer.h"
#include <stdio.h>
static void on_gesture(FVM_Context *ctx, const FVM_InputEvent *event,
                       FVM_InputPhase phase, FVM_MouseButton button, void *user) {
    (void)ctx; (void)phase; (void)user;
    printf("gesture=%d fingers=%u mapped_mouse=%d\n",
           (int)event->gesture, event->fingers, (int)button);
}
int main(void) {
    FVM_Config config; FVM_Runtime *runtime=0; FVM_Context *context=0;
    FVM_GestureRecognizer recognizer; FVM_InputRule rule; int dispatched=0;
    if (fvm_config_init(&config)!=FVM_OK ||
        fvm_runtime_create(&config,&runtime)!=FVM_OK) return 1;
    if (fvm_context_create(runtime,&context)!=FVM_OK) {
        fvm_runtime_destroy(runtime); return 1;
    }
    /* Two-finger tap behaves like a right mouse click. */
    rule.struct_size=sizeof(rule); rule.gesture=FVM_GESTURE_TAP;
    rule.fingers=2; rule.direction=0; rule.button=FVM_MOUSE_RIGHT;
    fvm_input_set_rule(context,&rule);
    fvm_input_set_callback(context,on_gesture,0);
    recognizer.struct_size=sizeof(recognizer);
    fvm_gesture_recognizer_init(&recognizer);
    fvm_gesture_recognizer_feed(context,&recognizer,FVM_TOUCH_BEGIN,2,10,20,100,&dispatched);
    fvm_gesture_recognizer_feed(context,&recognizer,FVM_TOUCH_END,0,11,20,160,&dispatched);
    fvm_context_destroy(context); fvm_runtime_destroy(runtime);
    return dispatched ? 0 : 1;
}
