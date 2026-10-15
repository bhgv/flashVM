/* Native callback exceptions stay inside the C boundary and cannot poison deferred dispatch. */
#include "flashvm/input_queue.h"
#include "flashvm/gc.h"
#include "../src/core/runtime.hpp"
#include <cassert>
#include <new>
static unsigned callback_mode,callback_calls;
static double last_x;
static void receive(FVM_Context *c,const FVM_InputEvent *event,FVM_InputPhase,FVM_MouseButton,void *)
{
 double x,y;assert(fvm_input_get_pointer(c,&x,&y)==FVM_OK && x==event->x && y==event->y);
 ++callback_calls;last_x=event->x;assert(fvm_gc_collect(c,0)==FVM_OK);
 if(c->pumping_input){unsigned nested=99;assert(fvm_input_pump(c,1,&nested)==FVM_INVALID_ARGUMENT && !nested && c->pumping_input);}
 if(callback_mode==3){FVM_InputEvent next=*event;next.x=9;assert(fvm_input_enqueue(c,&next,FVM_INPUT_END)==FVM_OK);throw 17;}
 if(callback_mode==1)throw std::bad_alloc();if(callback_mode==0)throw 17;
}
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_InputEvent event;unsigned pending,processed,before;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_input_set_callback(c,receive,0)==FVM_OK);
 event.gesture=FVM_GESTURE_TAP;event.fingers=1;event.direction=0;event.x=7;event.y=8;FVM_GCRootFrame *roots=c->gc_frames;
 for(unsigned mode=0;mode<3;++mode){callback_mode=mode;before=callback_calls;assert(fvm_input_dispatch(c,&event,FVM_INPUT_END)==(mode==0?FVM_ERROR:mode==1?FVM_OUT_OF_MEMORY:FVM_OK) && callback_calls==before+1 && !c->pumping_input && c->gc_frames==roots && !c->gc_native_depth && !c->script_depth);}
 for(unsigned mode=0;mode<4;++mode)
 {
  if(mode==2)continue;event.x=7;assert(fvm_input_enqueue(c,&event,FVM_INPUT_END)==FVM_OK);event.x=8;assert(fvm_input_enqueue(c,&event,FVM_INPUT_END)==FVM_OK);
  callback_mode=mode;processed=99;before=callback_calls;
  assert(fvm_input_pump(c,0,&processed)==(mode==1?FVM_OUT_OF_MEMORY:FVM_ERROR) && !processed && callback_calls==before+1 && last_x==7 && !c->pumping_input && c->gc_frames==roots);
  assert(fvm_input_pending(c,&pending)==FVM_OK && pending==(mode==3?2U:1U));callback_mode=2;
  assert(fvm_input_pump(c,0,&processed)==FVM_OK && processed==pending && last_x==(mode==3?9:8) && !c->pumping_input && fvm_input_pending(c,&pending)==FVM_OK && !pending);
 }
 fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
