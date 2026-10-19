/* Registration and poll OOM preserve pending records until their documented commit boundaries. */
#define main allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#include "timers_fixture.hpp"
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned operation=0;operation<3;++operation) {
        bool success=false;unsigned failures=0;
        for(long boundary=0;boundary<512 && !success;++boundary) {
            FVM_Context *c=timer_context(rt);TimerProbe probe;FVM_Value args[]={timer_function(c,&probe),timer_number(1),timer_number(0)};
            assert(fvm_value_string(&args[2],"borrowed timer argument")==FVM_OK);
            FVM_Value out;
            if(operation)timer_call(c,operation==1?"setTimeout":"setInterval",3,args);
            timer_sample=1001;unsigned delivered=777;allowance=boundary;
            FVM_Result r=operation?fvm_timers_poll(c,&delivered):fvm_call(c,"setTimeout",3,args,&out);allowance=-1;
            assert(!c->gc_frames && !c->gc_native_depth && !c->timers_dispatching && !c->avm1_execution_depth);
            if(r==FVM_OK) {success=true;if(operation)assert(delivered==1 && probe.calls==1);else assert(out.type==FVM_VALUE_NUMBER && c->scheduled_calls.size()==1);}
            else {
                assert(r==FVM_OUT_OF_MEMORY);++failures;
                if(!operation)assert(c->scheduled_calls.empty() && !c->last_timer_id && out.type==FVM_VALUE_UNDEFINED);
                else if(!delivered)assert(c->scheduled_calls.size()==1 && !probe.calls);
                else assert(delivered==1 && !probe.calls && c->scheduled_calls.size()==(operation==1?0U:1U));
            }
            assert(fvm_gc_collect(c,0)==FVM_OK);
            if(operation && r!=FVM_OK && !delivered)assert(timer_poll(c,1001)==1 && probe.calls==1);
            fvm_context_destroy(c);
        }
        assert(success && failures);
    }
    fvm_runtime_destroy(rt);
}
