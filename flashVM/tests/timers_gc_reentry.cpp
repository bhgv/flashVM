/* Pending and active GC roots survive self/other cancellation, registration, reentry and errors. */
#include "timers_fixture.hpp"
struct DeliveryState { unsigned calls;FVM_Value own,other,spawn;bool mutate;FVM_Result result;DeliveryState():calls(0),mutate(false),result(FVM_OK){} };
static FVM_Result body(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *raw)
{
    DeliveryState *s=(DeliveryState *)raw;++s->calls;
    assert(frame->argc==2 && frame->argv[0].type==FVM_VALUE_OBJECT && frame->argv[1].type==FVM_VALUE_STRING);
    if(s->mutate) {
        timer_call(c,"clearInterval",1,&s->own);timer_call(c,"clearInterval",1,&s->other);
        timer_sample+=100;FVM_Value a[]={s->spawn,timer_number(1)};timer_call(c,"setTimeout",2,a);
        unsigned nested=77;assert(fvm_timers_poll(c,&nested)==FVM_INVALID_ARGUMENT && nested==77);
        fvm_context_destroy(c);assert(c->timers_dispatching && !c->destroying);
    }
    assert(!c->gc_native_depth && fvm_gc_collect(c,0)==FVM_OK);
    FVM_Value v;assert(fvm_object_get(c,frame->argv[0].object,"retained",&v)==FVM_OK && v.number==42);
    const char expected[]={'a',0,'b'};assert(fvm_value_string_size(&frame->argv[1])==3 && !std::memcmp(frame->argv[1].string,expected,3));
    fvm_value_undefined(out);
    if(s->result==FVM_EXCEPTION) {FVM_Value thrown=timer_number(99);assert(fvm_exception_raise(c,&thrown)==FVM_EXCEPTION);}
    return s->result;
}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned repeating=0;repeating<2;++repeating)
    for(unsigned failure=0;failure<3;++failure) {
        FVM_Context *c=timer_context(rt);assert(fvm_gc_string_mode(c,FVM_GC_STRINGS_TRACED)==FVM_OK);DeliveryState state;state.mutate=true;state.result=failure==0?FVM_OK:failure==1?FVM_ERROR:FVM_EXCEPTION;
        TimerProbe spawned,skipped;state.spawn=timer_function(c,&spawned);assert(fvm_gc_pin(c,state.spawn.object)==FVM_OK);
        FVM_Object *fn=0,*arg=0;const unsigned char empty[]={0};assert(fvm_function_create_avm1(c,empty,sizeof(empty),0,0,0,&fn)==FVM_OK && fvm_function_set_body(c,fn,body,&state,0)==FVM_OK);
        assert(fvm_object_create(c,&arg)==FVM_OK);FVM_Value retained=timer_number(42);assert(fvm_object_set(c,arg,"retained",&retained)==FVM_OK);
        char borrowed[]={'a',0,'b'};FVM_Value values[]={timer_object(fn),timer_number(5),timer_object(arg),timer_number(0)};
        assert(fvm_value_string_bytes(&values[3],borrowed,sizeof(borrowed))==FVM_OK);
        state.own=timer_call(c,repeating?"setInterval":"setTimeout",4,values);std::memset(borrowed,'x',sizeof(borrowed));
        FVM_Value later[]={timer_function(c,&skipped),timer_number(5)};state.other=timer_call(c,"setTimeout",2,later);
        assert(fvm_gc_collect(c,0)==FVM_OK);assert(timer_poll(c,1005,state.result)==1 && state.calls==1 && !skipped.calls && !spawned.calls);
        if(state.result==FVM_EXCEPTION)assert(fvm_exception_clear(c)==FVM_OK);
        assert(c->scheduled_calls.size()==1);assert(timer_poll(c,1106)==1 && spawned.calls==1);assert(c->scheduled_calls.empty());
        assert(fvm_gc_collect(c,0)==FVM_OK);assert(fvm_gc_unpin(c,state.spawn.object)==FVM_OK);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
