/* Preparation is atomic; committed timeout consumption survives an exhausted poll budget. */
#include "timers_fixture.hpp"
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    FVM_Context *c=timer_context(rt);TimerProbe first,second;FVM_Value args[]={timer_function(c,&first),timer_number(1),timer_number(0)};
    assert(fvm_value_string(&args[2],"long borrowed argument")==FVM_OK);
    rt->config.instruction_limit=2;timer_call(c,"setTimeout",3,args,FVM_ERROR);assert(c->scheduled_calls.empty() && !c->last_timer_id);
    rt->config.instruction_limit=0;FVM_Value id=timer_call(c,"setTimeout",2,args);FVM_Value b[]={timer_function(c,&second),timer_number(1)};timer_call(c,"setTimeout",2,b);
    rt->config.instruction_limit=1;assert(timer_poll(c,1001,FVM_ERROR)==0 && c->scheduled_calls.size()==2 && !first.calls && !second.calls);
    rt->config.instruction_limit=2;timer_call(c,"clearTimeout",1,&id,FVM_ERROR);assert(c->scheduled_calls.size()==2);
    rt->config.instruction_limit=7;assert(timer_poll(c,1001,FVM_ERROR)==1 && first.calls==1 && !second.calls && c->scheduled_calls.size()==1);
    rt->config.instruction_limit=0;assert(timer_poll(c,1001)==1 && second.calls==1 && c->scheduled_calls.empty());
    /* Registration cap and rejected arguments must not consume an ID or replace live calls. */
    for(unsigned i=0;i<1024;++i)timer_call(c,"setInterval",2,args);
    unsigned long last=c->last_timer_id;timer_call(c,"setInterval",2,args,FVM_ERROR);assert(c->last_timer_id==last && c->scheduled_calls.size()==1024);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);
}
