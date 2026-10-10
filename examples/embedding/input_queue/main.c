/* C89 example: feed recognized gestures to the deferred input queue.
 * This example runs on one thread. Do not enqueue from an OS callback thread.
 */
#include "flashvm/input_queue.h"
#include <stdio.h>
static void on_gesture(FVM_Context *ctx,const FVM_InputEvent *event,
                       FVM_InputPhase phase,FVM_MouseButton button,void *user) {
    (void)ctx; (void)user;
    printf("gesture=%d fingers=%u phase=%d mouse=%d\n",
           (int)event->gesture,event->fingers,(int)phase,(int)button);
}
int main(void) {
    FVM_Config config;
    FVM_Runtime *runtime=0;
    FVM_Context *context=0;
    FVM_InputEvent event;
    FVM_InputRule rule;
    unsigned int dispatched=0;
    if (fvm_config_init(&config)!=FVM_OK ||
        fvm_runtime_create(&config,&runtime)!=FVM_OK) return 1;
    if (fvm_context_create(runtime,&context)!=FVM_OK) {
        fvm_runtime_destroy(runtime); return 1;
    }
    rule.struct_size=sizeof(rule); rule.gesture=FVM_GESTURE_TAP;
    rule.fingers=2; rule.direction=0; rule.button=FVM_MOUSE_RIGHT;
    fvm_input_set_rule(context,&rule);
    fvm_input_set_callback(context,on_gesture,0);
    event.gesture=FVM_GESTURE_TAP; event.fingers=2;
    event.direction=0; event.x=100; event.y=200;
    fvm_input_enqueue(context,&event,FVM_INPUT_END);
    /* Call pump on the owning VM thread, typically once per frame. */
    fvm_input_pump(context,0,&dispatched);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    return dispatched==1 ? 0 : 1;
}
