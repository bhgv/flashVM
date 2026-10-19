/* Clock/error/work fixtures prove capped ceil hints without consuming or invoking pending calls. */
#include "timers_fixture.hpp"
#include <climits>
static void hint(FVM_Context *c,unsigned long cap,unsigned long expected,int expected_pending)
{
    unsigned long wait=777;int pending=99;assert(fvm_timers_next_delay(c,cap,&wait,&pending)==FVM_OK);
    assert(wait==expected && pending==expected_pending && !c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);
}
static FVM_Result query_inside(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{
    assert(c->timers_dispatching);unsigned long wait=777;int pending=99;unsigned long before=c->avm1_steps;
    assert(fvm_timers_next_delay(c,1000,&wait,&pending)==FVM_OK && pending==1 && wait==99);
    assert(c->avm1_steps>before);fvm_value_undefined(out);return FVM_OK;
}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    FVM_Context *c=timer_context(rt);timer_available=0;hint(c,ULONG_MAX,ULONG_MAX,0);hint(c,0,0,0);timer_available=1;
    TimerProbe probe;FVM_Value args[]={timer_function(c,&probe),timer_number(100)};
    FVM_Value interval=timer_call(c,"setInterval",2,args);args[1]=timer_number(10);timer_call(c,"setTimeout",2,args);
    timer_sample=1000.25;hint(c,1000,10,1);hint(c,7,7,1);hint(c,0,0,1);
    timer_sample=1009.999;hint(c,1000,1,1);timer_sample=1010;hint(c,1000,0,1);
    assert(!probe.calls && c->scheduled_calls.size()==2);assert(timer_poll(c,1010)==1 && probe.calls==1);
    hint(c,1000,90,1);timer_sample=1005;hint(c,1000,90,1);
    unsigned long wait=777;int pending=99;
    timer_available=0;assert(fvm_timers_next_delay(c,1000,&wait,&pending)==FVM_ERROR && wait==777 && pending==99);timer_available=1;
    for(unsigned phase=0;phase<3;++phase) {
        if(!phase)c->gc_collecting=true;else if(phase==1)c->gc_releasing=true;else c->destroying=true;
        assert(fvm_timers_next_delay(c,1000,&wait,&pending)==FVM_INVALID_ARGUMENT && wait==777 && pending==99);
        c->gc_collecting=c->gc_releasing=c->destroying=false;
    }
    assert(fvm_timers_next_delay(0,1000,&wait,&pending)==FVM_INVALID_ARGUMENT && wait==777 && pending==99);
    assert(fvm_timers_next_delay(c,1000,0,&pending)==FVM_INVALID_ARGUMENT && pending==99);
    assert(fvm_timers_next_delay(c,1000,&wait,0)==FVM_INVALID_ARGUMENT && wait==777);
    timer_sample=1000+4294967296.0;hint(c,ULONG_MAX,0,1);timer_call(c,"clearInterval",1,&interval);
    args[1]=timer_number(100);interval=timer_call(c,"setInterval",2,args);hint(c,ULONG_MAX,100,1);
    timer_call(c,"setTimeout",2,args);timer_call(c,"setTimeout",2,args);rt->config.instruction_limit=2;
    assert(fvm_timers_next_delay(c,1000,&wait,&pending)==FVM_ERROR && wait==777 && pending==99 && c->scheduled_calls.size()==3);
    rt->config.instruction_limit=0;hint(c,1000,100,1);fvm_context_destroy(c);
    /* At 2^53 ms, storing an odd allowed delay rounds to a 2^31 ms representable deadline. */
    c=timer_context(rt);args[0]=timer_function(c,&probe);args[1]=timer_number(2147483647.0);
    timer_sample=1000+9007199254740992.0;timer_call(c,"setInterval",2,args);
    hint(c,ULONG_MAX,2147483648UL,1);hint(c,1000,1000,1);fvm_context_destroy(c);
    /* The query is safe during callbacks and shares their current work scope. */
    c=timer_context(rt);FVM_Object *fn=0;assert(fvm_function_create_native(c,query_inside,0,0,&fn)==FVM_OK);
    args[0]=timer_object(fn);args[1]=timer_number(1);timer_call(c,"setTimeout",2,args);
    args[0]=timer_function(c,&probe);args[1]=timer_number(100);timer_call(c,"setInterval",2,args);
    assert(timer_poll(c,1001)==1);fvm_context_destroy(c);fvm_runtime_destroy(rt);
}
