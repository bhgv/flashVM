#include "flashvm/flashvm.h"
#include "flashvm/input.h"
#include <assert.h>
static int called=0;
static void on_input(FVM_Context *ctx, const FVM_InputEvent *event, FVM_InputPhase phase, FVM_MouseButton button, void *userdata) {
    (void)ctx; (void)userdata;
    assert(event->fingers==2);
    assert(phase==FVM_INPUT_END);
    assert(button==FVM_MOUSE_RIGHT);
    ++called;
}
int main(void) {
    FVM_Config cfg; FVM_Runtime *runtime=0; FVM_Context *ctx=0;
    FVM_InputRule rule; FVM_InputEvent event;
    assert(fvm_config_init(&cfg)==FVM_OK);
    assert(fvm_runtime_create(&cfg,&runtime)==FVM_OK);
    assert(fvm_context_create(runtime,&ctx)==FVM_OK);
    rule.struct_size=sizeof(rule); rule.gesture=FVM_GESTURE_TAP; rule.fingers=2; rule.direction=0; rule.button=FVM_MOUSE_RIGHT;
    assert(fvm_input_set_rule(ctx,&rule)==FVM_OK);
    event.gesture=FVM_GESTURE_TAP; event.fingers=2; event.direction=0; event.x=20; event.y=30;
    assert(fvm_input_set_callback(ctx,on_input,0)==FVM_OK);
    assert(fvm_input_dispatch(ctx,&event,FVM_INPUT_END)==FVM_OK);
    assert(called==1);
    fvm_context_destroy(ctx); fvm_runtime_destroy(runtime);
    return 0;
}
