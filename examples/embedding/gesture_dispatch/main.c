/* C89 example: map a two-finger tap to right mouse button and observe it. */
#include "flashvm/flashvm.h"
#include "flashvm/input.h"
#include <stdio.h>
static void on_gesture(FVM_Context *context, const FVM_InputEvent *event,
                       FVM_InputPhase phase, FVM_MouseButton button, void *userdata) {
    (void)context; (void)userdata;
    printf("gesture=%d fingers=%u phase=%d mouse=%d\n",
           (int)event->gesture,event->fingers,(int)phase,(int)button);
}
int main(void) {
    FVM_Config config; FVM_Runtime *runtime=0; FVM_Context *context=0;
    FVM_InputRule rule; FVM_InputEvent event;
    if (fvm_config_init(&config)!=FVM_OK ||
        fvm_runtime_create(&config,&runtime)!=FVM_OK) return 1;
    if (fvm_context_create(runtime,&context)!=FVM_OK) { fvm_runtime_destroy(runtime); return 1; }
    rule.struct_size=sizeof(rule); rule.gesture=FVM_GESTURE_TAP;
    rule.fingers=2; rule.direction=0; rule.button=FVM_MOUSE_RIGHT;
    fvm_input_set_rule(context,&rule);
    fvm_input_set_callback(context,on_gesture,0);
    event.gesture=FVM_GESTURE_TAP; event.fingers=2;
    event.direction=0; event.x=100; event.y=50;
    fvm_input_dispatch(context,&event,FVM_INPUT_END);
    fvm_context_destroy(context); fvm_runtime_destroy(runtime);
    return 0;
}
