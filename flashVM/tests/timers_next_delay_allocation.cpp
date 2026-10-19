/* Deadline queries allocate no storage, retain no roots and preserve every scheduled deadline. */
#define main allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#include "timers_fixture.hpp"
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    FVM_Context *c=timer_context(rt);TimerProbe probe;FVM_Value args[]={timer_function(c,&probe),timer_number(10)};
    FVM_Value id=timer_call(c,"setInterval",2,args);timer_sample=1005.5;
    FVM_GCRootFrame *roots=c->gc_frames;size_t objects=c->objects.size(),strings=c->strings.size();
    allowance=0;
    for(unsigned i=0;i<128;++i) {
        unsigned long wait=0;int pending=0;assert(fvm_timers_next_delay(c,1000,&wait,&pending)==FVM_OK && wait==5 && pending==1);
    }
    allowance=-1;assert(c->gc_frames==roots && !c->avm1_execution_depth && c->objects.size()==objects && c->strings.size()==strings);
    assert(c->scheduled_calls.size()==1 && !probe.calls && c->scheduled_calls[(unsigned long)id.number]->deadline==10);
    assert(timer_poll(c,1010)==1 && probe.calls==1);fvm_context_destroy(c);fvm_runtime_destroy(rt);
}
