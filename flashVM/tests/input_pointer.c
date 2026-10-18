/* C89 world pointer state is context-local, atomic on rejection and delivered before callbacks. */
#include "flashvm/input_queue.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
static unsigned calls;
static void position(FVM_Context *c,double x,double y)
{double a=73,b=74;assert(fvm_input_get_pointer(c,&a,&b)==FVM_OK && a==x && b==y);}
static void receive(FVM_Context *c,const FVM_InputEvent *e,FVM_InputPhase phase,FVM_MouseButton button,void *raw)
{(void)phase;(void)button;(void)raw;position(c,e->x,e->y);++calls;}
int main(void)
{
 FVM_Config cfg;FVM_Runtime *rt;FVM_Context *c,*other;FVM_InputEvent event;double x=73,y=74;unsigned processed;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_context_create(rt,&other)==FVM_OK);
 position(c,0,0);position(other,0,0);assert(fvm_input_set_pointer(c,3,5)==FVM_OK);position(c,3,5);position(other,0,0);
 assert(fvm_input_get_pointer(0,&x,&y)==FVM_INVALID_ARGUMENT && x==73 && y==74 && fvm_input_get_pointer(c,0,&y)==FVM_INVALID_ARGUMENT && y==74 && fvm_input_get_pointer(c,&x,&x)==FVM_INVALID_ARGUMENT && x==73);
 assert(fvm_input_set_pointer(c,HUGE_VAL,0)==FVM_INVALID_ARGUMENT && fvm_input_set_pointer(c,0,-HUGE_VAL)==FVM_INVALID_ARGUMENT && fvm_input_set_pointer(c,strtod("NaN",0),0)==FVM_INVALID_ARGUMENT && fvm_input_set_pointer(c,0,strtod("NaN",0))==FVM_INVALID_ARGUMENT);position(c,3,5);
 assert(fvm_input_set_pointer(c,DBL_MAX,-DBL_MAX)==FVM_OK);position(c,DBL_MAX,-DBL_MAX);assert(fvm_input_set_pointer(c,3,5)==FVM_OK && fvm_input_set_callback(c,receive,0)==FVM_OK);
 event.gesture=FVM_GESTURE_TAP;event.fingers=1;event.direction=0;event.x=7;event.y=9;
 assert(fvm_input_enqueue(c,&event,FVM_INPUT_END)==FVM_OK && !calls);position(c,3,5);
 assert(fvm_input_pump(c,0,&processed)==FVM_OK && processed==1 && calls==1);position(c,7,9);
 event.x=11;event.y=13;assert(fvm_input_dispatch(c,&event,FVM_INPUT_UPDATE)==FVM_OK && calls==2);position(c,11,13);
 event.x=17;event.fingers=0;assert(fvm_input_dispatch(c,&event,FVM_INPUT_END)==FVM_INVALID_ARGUMENT && calls==2);position(c,11,13);
 event.fingers=1;assert(fvm_input_dispatch(c,&event,(FVM_InputPhase)0)==FVM_INVALID_ARGUMENT && calls==2);position(c,11,13);position(other,0,0);
 fvm_context_destroy(other);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
