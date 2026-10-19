/* Duplicate focus/listener routes, inherited eligibility, collecting bodies and error restoration. */
#include "timers_fixture.hpp"
#include "flashvm/stage.h"
#include "flashvm/mouse.h"
#include "flashvm/keyboard.h"
#include "flashvm/movieclip.h"
struct State{unsigned calls;bool nested;FVM_Result result;State():calls(0),nested(false),result(FVM_OK){}};
static FVM_Result focus_call(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *raw)
{State *s=(State *)raw;++s->calls;assert(c->stage_update_allowed==(s->calls==1));assert(fvm_call(c,"updateAfterEvent",0,0,out)==FVM_OK);int pending=77;assert(fvm_stage_take_update(c,&pending)==FVM_OK && pending==(s->calls==1));return FVM_OK;}
static FVM_Result wheel(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *raw)
{State *s=(State *)raw;assert(c->stage_update_allowed==s->nested);++s->calls;return fvm_call(c,"updateAfterEvent",0,0,out);}
static FVM_Result collect_body(FVM_Context *c,const FVM_ScriptFrame *,FVM_Value *out,void *raw)
{
    State *s=(State *)raw;assert(c->stage_update_allowed && !c->gc_native_depth);assert(fvm_gc_collect(c,0)==FVM_OK);
    s->nested=true;assert(fvm_input_dispatch_mouse_wheel(c,1,0,0)==FVM_OK && c->stage_update_allowed);s->nested=false;
    assert(fvm_call(c,"updateAfterEvent",0,0,out)==FVM_OK && fvm_gc_collect(c,0)==FVM_OK && c->stage_update_pending);
    if(s->result==FVM_EXCEPTION){FVM_Value v=timer_number(99);return fvm_exception_raise(c,&v);}return s->result;
}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    FVM_Context *c=timer_context(rt);State focus;FVM_Object *clip=0;assert(fvm_movieclip_create(c,"focused",1,&clip)==FVM_OK && fvm_input_set_keyboard_focus(c,clip)==FVM_OK && fvm_input_add_key_listener(c,clip)==FVM_OK && fvm_object_define_method_rooted(c,clip,"onKeyDown",focus_call,&focus,0)==FVM_OK);FVM_KeyboardState keys=c->keyboard;keys.down[FVM_KEY_SPACE]=1;assert(fvm_input_dispatch_keyboard(c,&keys,1)==FVM_OK && focus.calls==2 && !c->stage_update_allowed && !c->stage_update_pending);fvm_context_destroy(c);
    for(unsigned failure=0;failure<3;++failure){c=timer_context(rt);State state;state.result=failure==0?FVM_OK:failure==1?FVM_ERROR:FVM_EXCEPTION;FVM_Object *listener=0;assert(fvm_object_create(c,&listener)==FVM_OK && fvm_object_define_method_rooted(c,listener,"onMouseWheel",wheel,&state,0)==FVM_OK && fvm_input_add_mouse_listener(c,listener)==FVM_OK);
        const unsigned char empty[]={0};FVM_Object *fn=0;assert(fvm_function_create_avm1(c,empty,sizeof(empty),0,0,0,&fn)==FVM_OK && fvm_function_set_body(c,fn,collect_body,&state,0)==FVM_OK);FVM_Value args[]={timer_object(fn),timer_number(1)};FVM_Value id=timer_call(c,"setInterval",2,args);
        assert(timer_poll(c,1001,state.result)==1 && state.calls==1 && !c->stage_update_allowed && c->stage_update_pending);if(state.result==FVM_EXCEPTION)assert(fvm_exception_clear(c)==FVM_OK);timer_call(c,"clearInterval",1,&id);int pending=0;assert(fvm_stage_take_update(c,&pending)==FVM_OK && pending==1);
        assert(fvm_input_dispatch_mouse_wheel(c,1,0,0)==FVM_OK && !c->stage_update_pending && !c->stage_update_allowed && state.calls==2);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
