#include "flashvm/gesture_recognizer.h"
#include <assert.h>
static int calls=0;
static void on_event(FVM_Context *ctx,const FVM_InputEvent *e,FVM_InputPhase phase,FVM_MouseButton button,void *ud) {
    (void)ctx;(void)ud;
    assert(phase==FVM_INPUT_END);
    if (calls==0) { assert(e->gesture==FVM_GESTURE_TAP); assert(e->fingers==2); assert(button==FVM_MOUSE_RIGHT); }
    else { assert(e->gesture==FVM_GESTURE_SWIPE); assert(e->direction==1); assert(button==FVM_MOUSE_WHEEL_DOWN); }
    ++calls;
}
int main(void) {
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *ctx=0;
    FVM_GestureRecognizer rec; FVM_InputRule rule; int dispatched=0;
    assert(fvm_config_init(&cfg)==FVM_OK);
    assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&ctx)==FVM_OK);
    assert(fvm_input_set_callback(ctx,on_event,0)==FVM_OK);
    rule.struct_size=sizeof(rule); rule.gesture=FVM_GESTURE_TAP; rule.fingers=2; rule.direction=0; rule.button=FVM_MOUSE_RIGHT;
    assert(fvm_input_set_rule(ctx,&rule)==FVM_OK);
    rule.gesture=FVM_GESTURE_SWIPE; rule.direction=1; rule.button=FVM_MOUSE_WHEEL_DOWN;
    assert(fvm_input_set_rule(ctx,&rule)==FVM_OK);
    rec.struct_size=sizeof(rec); assert(fvm_gesture_recognizer_init(&rec)==FVM_OK);
    assert(fvm_gesture_recognizer_feed(ctx,&rec,FVM_TOUCH_BEGIN,1,0,0,100,&dispatched)==FVM_OK);
    assert(fvm_gesture_recognizer_feed(ctx,&rec,FVM_TOUCH_MOVE,2,1,1,120,&dispatched)==FVM_OK);
    assert(fvm_gesture_recognizer_feed(ctx,&rec,FVM_TOUCH_END,0,2,2,150,&dispatched)==FVM_OK);
    assert(dispatched==1);
    assert(fvm_gesture_recognizer_feed(ctx,&rec,FVM_TOUCH_BEGIN,2,0,0,200,&dispatched)==FVM_OK);
    assert(fvm_gesture_recognizer_feed(ctx,&rec,FVM_TOUCH_END,0,100,0,400,&dispatched)==FVM_OK);
    assert(dispatched==1 && calls==2);
    assert(fvm_gesture_recognizer_feed(ctx,&rec,FVM_TOUCH_BEGIN,1,0,0,500,&dispatched)==FVM_OK);
    assert(fvm_gesture_recognizer_feed(ctx,&rec,FVM_TOUCH_CANCEL,0,0,0,510,&dispatched)==FVM_OK);
    assert(calls==2);
    fvm_context_destroy(ctx); fvm_runtime_destroy(rt); return 0;
}
