/* Context-local requested cursor state, invalid output atomicity, lifecycle and callback use. */
#include "../src/core/runtime.hpp"
#include "flashvm/mouse.h"
#include "flashvm/input_queue.h"
#include "flashvm/gc.h"
#include <cassert>
static FVM_Result hide(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{
    int previous=-1,visible=-1;
    assert(c->mouse_dispatching && fvm_input_set_cursor_visible(c,0,&previous)==FVM_OK && previous==1);
    assert(fvm_gc_collect(c,0)==FVM_OK);
    assert(fvm_input_get_cursor_visible(c,&visible)==FVM_OK && !visible);
    fvm_value_undefined(out);return FVM_OK;
}
int main()
{
    FVM_Config cfg;FVM_Runtime *r=0;FVM_Context *c=0,*other=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&r)==FVM_OK);
    assert(fvm_context_create(r,&c)==FVM_OK && fvm_context_create(r,&other)==FVM_OK);
    int visible=-1,previous=-1;
    assert(fvm_input_get_cursor_visible(c,&visible)==FVM_OK && visible==1);
    assert(fvm_input_set_cursor_visible(c,0,&previous)==FVM_OK && previous==1);
    assert(fvm_input_set_cursor_visible(c,0,&previous)==FVM_OK && previous==0);
    assert(fvm_input_get_cursor_visible(other,&visible)==FVM_OK && visible==1);
    previous=23;visible=24;
    assert(fvm_input_set_cursor_visible(c,2,&previous)==FVM_INVALID_ARGUMENT && previous==23);
    assert(fvm_input_set_cursor_visible(c,-1,&previous)==FVM_INVALID_ARGUMENT && previous==23);
    assert(fvm_input_set_cursor_visible(c,1,0)==FVM_INVALID_ARGUMENT && !c->cursor_visible);
    assert(fvm_input_set_cursor_visible(0,1,&previous)==FVM_INVALID_ARGUMENT && previous==23);
    assert(fvm_input_get_cursor_visible(0,&visible)==FVM_INVALID_ARGUMENT && visible==24);
    assert(fvm_input_get_cursor_visible(c,0)==FVM_INVALID_ARGUMENT);
    for(unsigned i=0;i<3;++i) {
        c->gc_collecting=i==0;c->gc_releasing=i==1;c->destroying=i==2;
        assert(fvm_input_set_cursor_visible(c,1,&previous)==FVM_INVALID_ARGUMENT && previous==23);
        assert(fvm_input_get_cursor_visible(c,&visible)==FVM_INVALID_ARGUMENT && visible==24);
        assert(!c->cursor_visible);
    }
    c->gc_collecting=c->gc_releasing=c->destroying=false;
    unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
    assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
    assert(fvm_input_get_cursor_visible(c,&visible)==FVM_OK && visible==0);
    assert(fvm_input_set_cursor_visible(c,1,&previous)==FVM_OK && previous==0);
    FVM_Object *o=0;assert(fvm_object_create(c,&o)==FVM_OK);
    assert(fvm_object_define_method_rooted(c,o,"onMouseDown",hide,0,0)==FVM_OK);
    o->methods["onMouseDown"].collector_entry=true;
    assert(fvm_input_add_mouse_listener(c,o)==FVM_OK);
    assert(fvm_input_enqueue_mouse(c,FVM_MOUSE_EVENT_MOVE,12,13)==FVM_OK);
    assert(fvm_input_dispatch_mouse(c,FVM_MOUSE_EVENT_DOWN,7,8)==FVM_OK);
    assert(!c->cursor_visible && c->pointer_world_x==7 && c->pointer_world_y==8);
    unsigned pending=0;assert(fvm_input_pending(c,&pending)==FVM_OK && pending==1);
    assert(fvm_input_set_cursor_visible(c,1,&previous)==FVM_OK && previous==0);
    assert(fvm_input_pending(c,&pending)==FVM_OK && pending==1 && c->mouse_listeners.size()==1);
    assert(c->pointer_world_x==7 && c->pointer_world_y==8 && !c->mouse_dispatching && !c->gc_frames);
    fvm_context_destroy(c);fvm_context_destroy(other);fvm_runtime_destroy(r);
}
