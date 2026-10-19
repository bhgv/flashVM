/* Wheel Number delivery, rooted mutation, cross-event recursion, lifecycle, drag and validation. */
#include "../src/core/runtime.hpp"
#include "flashvm/mouse.h"
#include "flashvm/gc.h"
#include "flashvm/standard.h"
#include "flashvm/movieclip.h"
#include <cassert>
#include <cstring>
#include <limits>
struct Trace {
    FVM_Object *first, *second, *late, *clip;
    FVM_Object *seen[16]; unsigned calls;
    double delta,x,y; bool mutate; FVM_Result status;
};
static FVM_Result wheel(FVM_Context *c,FVM_Object *self,unsigned argc,const FVM_Value *args,FVM_Value *out,void *raw)
{
    Trace *t=(Trace *)raw;
    assert(argc==1 && args && args[0].type==FVM_VALUE_NUMBER && args[0].number==t->delta);
    assert(t->calls<16); t->seen[t->calls++]=self;
    assert(c->pointer_world_x==t->x && c->pointer_world_y==t->y);
    FVM_Affine matrix; assert(fvm_movieclip_get_matrix(c,t->clip,&matrix)==FVM_OK);
    assert(matrix.x0==t->x && matrix.y0==t->y);
    assert(fvm_input_dispatch_mouse_wheel(c,1,999,888)==FVM_INVALID_ARGUMENT);
    assert(fvm_input_dispatch_mouse(c,FVM_MOUSE_EVENT_MOVE,999,888)==FVM_INVALID_ARGUMENT);
    if(t->mutate) {
        t->mutate=false; int removed;
        assert(fvm_input_remove_mouse_listener(c,t->first,&removed)==FVM_OK && removed);
        assert(fvm_input_remove_mouse_listener(c,t->second,&removed)==FVM_OK && removed);
        assert(fvm_input_add_mouse_listener(c,t->late)==FVM_OK);
        assert(!c->gc_native_depth && fvm_gc_collect(c,0)==FVM_OK);
        fvm_context_destroy(c); assert(c->mouse_dispatching);
    }
    fvm_value_undefined(out); return t->status;
}
static FVM_Result pointer(FVM_Context *c,FVM_Object *,unsigned n,const FVM_Value *,FVM_Value *out,void *)
{
    assert(n==0 && fvm_input_dispatch_mouse_wheel(c,1,0,0)==FVM_INVALID_ARGUMENT);
    fvm_value_undefined(out);return FVM_OK;
}
static FVM_Object *listener(FVM_Context *c,Trace *t,FVM_Object *capture)
{
    FVM_Object *o=0;assert(fvm_object_create(c,&o)==FVM_OK);
    assert(fvm_object_define_method_rooted(c,o,"onMouseWheel",wheel,t,capture)==FVM_OK);
    o->methods["onMouseWheel"].collector_entry=true;
    return o;
}
int main()
{
    FVM_Config cfg;FVM_Runtime *r=0;FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&r)==FVM_OK && fvm_context_create(r,&c)==FVM_OK);
    assert(fvm_standard_install(c)==FVM_OK && fvm_standard_mouse_install(c)==FVM_OK);
    Trace t;std::memset(&t,0,sizeof(t));t.status=FVM_OK;
    t.late=listener(c,&t,0);assert(fvm_gc_pin(c,t.late)==FVM_OK);
    t.first=listener(c,&t,t.late);assert(fvm_input_add_mouse_listener(c,t.first)==FVM_OK);
    t.second=listener(c,&t,0);assert(fvm_input_add_mouse_listener(c,t.second)==FVM_OK);
    assert(fvm_gc_unpin(c,t.late)==FVM_OK);
    assert(fvm_movieclip_create(c,"dragged",1,&t.clip)==FVM_OK);
    FVM_Value yes,out;fvm_value_boolean(&yes,1);
    assert(fvm_object_call(c,t.clip,"startDrag",1,&yes,&out)==FVM_OK);
    t.delta=-2.5;t.x=12;t.y=34;t.mutate=true;
    assert(fvm_input_dispatch_mouse_wheel(c,t.delta,t.x,t.y)==FVM_OK);
    assert(t.calls==2 && t.seen[0]==t.first && t.seen[1]==t.second);
    assert(c->mouse_listeners.size()==1 && c->mouse_listeners[0]==t.late);
    t.delta=0;t.x=-5;t.y=6;
    assert(fvm_input_dispatch_mouse_wheel(c,t.delta,t.x,t.y)==FVM_OK && t.calls==3);
    t.delta=3;t.status=FVM_UNSUPPORTED;t.x=9;t.y=10;
    assert(fvm_input_dispatch_mouse_wheel(c,t.delta,t.x,t.y)==FVM_UNSUPPORTED && t.calls==4);
    assert(c->pointer_world_x==9 && !c->mouse_dispatching && !c->gc_frames && !c->gc_native_depth);
    const double bad[]={std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()};
    for(unsigned i=0;i<2;++i) {
        assert(fvm_input_dispatch_mouse_wheel(c,bad[i],0,0)==FVM_INVALID_ARGUMENT);
        assert(fvm_input_dispatch_mouse_wheel(c,1,bad[i],0)==FVM_INVALID_ARGUMENT);
        assert(fvm_input_dispatch_mouse_wheel(c,1,0,bad[i])==FVM_INVALID_ARGUMENT);
    }
    assert(c->pointer_world_x==9 && t.calls==4);
    c->gc_collecting=true;assert(fvm_input_dispatch_mouse_wheel(c,1,0,0)==FVM_INVALID_ARGUMENT);c->gc_collecting=false;
    c->gc_releasing=true;assert(fvm_input_dispatch_mouse_wheel(c,1,0,0)==FVM_INVALID_ARGUMENT);c->gc_releasing=false;
    c->destroying=true;assert(fvm_input_dispatch_mouse_wheel(c,1,0,0)==FVM_INVALID_ARGUMENT);c->destroying=false;
    assert(fvm_input_dispatch_mouse_wheel(0,1,0,0)==FVM_INVALID_ARGUMENT);
    assert(fvm_object_define_method_rooted(c,t.late,"onMouseMove",pointer,0,0)==FVM_OK);
    assert(fvm_input_dispatch_mouse(c,FVM_MOUSE_EVENT_MOVE,2,3)==FVM_OK);
    FVM_Value zero;fvm_value_number(&zero,0);
    assert(fvm_object_set(c,t.late,"onMouseWheel",&zero)==FVM_OK);
    assert(fvm_register_function(c,"onMouseWheel",wheel,&t)==FVM_OK);
    assert(fvm_input_dispatch_mouse_wheel(c,1,4,5)==FVM_OK && t.calls==4);
    assert(fvm_exec_drag_end(c)==FVM_OK);
    fvm_context_destroy(c);fvm_runtime_destroy(r);
}
