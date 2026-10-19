/* Host requests and dynamic eligibility: intervals/Mouse/focused clips, not timeout/Key/wheel. */
#include "timers_fixture.hpp"
#include "flashvm/stage.h"
#include "flashvm/mouse.h"
#include "flashvm/keyboard.h"
#include "flashvm/movieclip.h"
struct UpdateProbe { unsigned calls;bool eligible;FVM_Result result;UpdateProbe():calls(0),eligible(false),result(FVM_OK){} };
static FVM_Result update(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *raw)
{
    UpdateProbe *s=(UpdateProbe *)raw;++s->calls;assert(c->stage_update_allowed==s->eligible);
    FVM_Value ignored;assert(fvm_call(c,"updateAfterEvent",0,0,&ignored)==FVM_OK && ignored.type==FVM_VALUE_UNDEFINED);
    fvm_value_undefined(out);if(s->result==FVM_EXCEPTION){FVM_Value v=timer_number(99);return fvm_exception_raise(c,&v);}return s->result;
}
static int take(FVM_Context *c)
{int result=77;assert(fvm_stage_take_update(c,&result)==FVM_OK && !c->stage_update_allowed);return result;}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=0;version<=8;++version) {
        if(version && version<4)continue;
        FVM_Context *c=timer_context(rt,version);
        for(unsigned route=0;route<2;++route)timer_call(c,"updateAfterEvent",0,0,version==4?FVM_UNSUPPORTED:FVM_OK,route);
        assert(!take(c));assert(fvm_stage_request_update(c)==FVM_OK && fvm_stage_request_update(c)==FVM_OK);
        int pending=77;assert(fvm_stage_get_update(c,&pending)==FVM_OK && pending==1);assert(take(c)==1 && !take(c));
        fvm_context_destroy(c);
    }
    for(unsigned kind=0;kind<3;++kind)
    for(unsigned status=0;status<3;++status) {
        FVM_Context *c=timer_context(rt);UpdateProbe probe;probe.eligible=kind!=0;probe.result=status==0?FVM_OK:status==1?FVM_ERROR:FVM_EXCEPTION;
        FVM_Object *fn=0;assert(fvm_function_create_native(c,update,&probe,0,&fn)==FVM_OK);
        FVM_Value args[]={timer_object(fn),timer_number(1)};
        if(kind==2){FVM_Object *owner=0;assert(fvm_object_create(c,&owner)==FVM_OK);assert(fvm_object_set(c,owner,"tick",args)==FVM_OK);args[0]=timer_object(owner);assert(fvm_value_string(&args[1],"tick")==FVM_OK);FVM_Value method_args[]={args[0],args[1],timer_number(1)};timer_call(c,"setInterval",3,method_args);}
        else timer_call(c,kind?"setInterval":"setTimeout",2,args);
        assert(timer_poll(c,1001,probe.result)==1 && probe.calls==1);assert(take(c)==(kind?1:0));
        if(probe.result==FVM_EXCEPTION)assert(fvm_exception_clear(c)==FVM_OK);fvm_context_destroy(c);
    }
    FVM_Context *c=timer_context(rt);UpdateProbe mouse;mouse.eligible=true;FVM_Object *listener=0;assert(fvm_object_create(c,&listener)==FVM_OK);
    const char *handlers[]={"onMouseMove","onMouseDown","onMouseUp","onMouseWheel"};for(unsigned i=0;i<4;++i)assert(fvm_object_define_method_rooted(c,listener,handlers[i],update,&mouse,0)==FVM_OK);
    assert(fvm_input_add_mouse_listener(c,listener)==FVM_OK);
    const FVM_MouseEventType kinds[]={FVM_MOUSE_EVENT_MOVE,FVM_MOUSE_EVENT_DOWN,FVM_MOUSE_EVENT_UP};
    for(unsigned i=0;i<3;++i){assert(fvm_input_dispatch_mouse(c,kinds[i],12,23)==FVM_OK && take(c)==1);}
    mouse.eligible=false;assert(fvm_input_dispatch_mouse_wheel(c,1,12,23)==FVM_OK && !take(c));
    mouse.eligible=true;mouse.result=FVM_ERROR;assert(fvm_input_dispatch_mouse(c,FVM_MOUSE_EVENT_MOVE,12,23)==FVM_ERROR && take(c)==1);
    FVM_Object *clip=0,*key_listener=0;UpdateProbe focus,key;focus.eligible=true;
    assert(fvm_movieclip_create(c,"focus",1,&clip)==FVM_OK && fvm_input_set_keyboard_focus(c,clip)==FVM_OK);
    assert(fvm_object_create(c,&key_listener)==FVM_OK);
    for(unsigned i=0;i<2;++i){const char *name=i?"onKeyUp":"onKeyDown";assert(fvm_object_define_method_rooted(c,clip,name,update,&focus,0)==FVM_OK && fvm_object_define_method_rooted(c,key_listener,name,update,&key,0)==FVM_OK);}
    assert(fvm_input_add_key_listener(c,key_listener)==FVM_OK);FVM_KeyboardState keys=c->keyboard;keys.last_code=FVM_KEY_SPACE;
    for(unsigned pressed=0;pressed<2;++pressed){keys.down[FVM_KEY_SPACE]=(unsigned char)pressed;assert(fvm_input_dispatch_keyboard(c,&keys,(int)pressed)==FVM_OK && take(c)==1);}
    assert(focus.calls==2 && key.calls==2);assert(fvm_input_set_keyboard_focus(c,0)==FVM_OK);assert(fvm_input_dispatch_keyboard(c,&keys,1)==FVM_OK && !take(c));
    assert(fvm_stage_request_update(c)==FVM_OK);int sentinel=77;
    for(unsigned phase=0;phase<3;++phase){if(!phase)c->gc_collecting=true;else if(phase==1)c->gc_releasing=true;else c->destroying=true;assert(fvm_stage_request_update(c)==FVM_INVALID_ARGUMENT && fvm_stage_get_update(c,&sentinel)==FVM_INVALID_ARGUMENT && fvm_stage_take_update(c,&sentinel)==FVM_INVALID_ARGUMENT && sentinel==77);c->gc_collecting=c->gc_releasing=c->destroying=false;}
    assert(take(c)==1 && fvm_stage_get_update(c,0)==FVM_INVALID_ARGUMENT && fvm_stage_take_update(c,0)==FVM_INVALID_ARGUMENT);
    assert(fvm_stage_get_update(0,&sentinel)==FVM_INVALID_ARGUMENT && sentinel==77);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);
}
