#include "flashvm/input_queue.h"
#include <assert.h>
static unsigned int calls=0;
static void on_input(FVM_Context *c,const FVM_InputEvent *e,FVM_InputPhase p,FVM_MouseButton b,void *u) {
    (void)c; (void)u;
    assert(e->x==(double)calls);
    assert(p==FVM_INPUT_END);
    assert(b==FVM_MOUSE_LEFT);
    ++calls;
}
int main(void) {
    FVM_Config cfg;
    FVM_Runtime *r=0;
    FVM_Context *c=0;
    FVM_InputRule rule;
    FVM_InputEvent event;
    unsigned int n=0;
    unsigned int i;
    assert(fvm_config_init(&cfg)==FVM_OK);
    assert(fvm_runtime_create(&cfg,&r)==FVM_OK);
    assert(fvm_context_create(r,&c)==FVM_OK);
    rule.struct_size=sizeof(rule); rule.gesture=FVM_GESTURE_TAP;
    rule.fingers=2; rule.direction=0; rule.button=FVM_MOUSE_LEFT;
    assert(fvm_input_set_rule(c,&rule)==FVM_OK);
    assert(fvm_input_set_callback(c,on_input,0)==FVM_OK);
    event.gesture=FVM_GESTURE_TAP; event.fingers=2;
    event.direction=0; event.y=12;
    for (i=0;i<3;++i) {
        event.x=(double)i;
        assert(fvm_input_enqueue(c,&event,FVM_INPUT_END)==FVM_OK);
    }
    assert(fvm_input_pending(c,&n)==FVM_OK && n==3);
    assert(fvm_input_pump(c,2,&n)==FVM_OK && n==2 && calls==2);
    assert(fvm_input_pump(c,0,&n)==FVM_OK && n==1 && calls==3);
    assert(fvm_input_pending(c,&n)==FVM_OK && n==0);
    assert(fvm_input_enqueue(c,&event,(FVM_InputPhase)99)==FVM_INVALID_ARGUMENT);
    fvm_context_destroy(c);
    fvm_runtime_destroy(r);
    return 0;
}
