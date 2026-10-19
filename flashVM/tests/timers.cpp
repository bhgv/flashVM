/* Availability, ID stability, delay/clock policy, dynamic method lookup and cancellation. */
#include "timers_fixture.hpp"
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=0;version<=8;++version) {
        if(version && version<4)continue;
        for(unsigned route=0;route<2;++route) {
            FVM_Context *c=timer_context(rt,version);TimerProbe probe;FVM_Value args[]={timer_function(c,&probe),timer_number(10)};
            const char *names[]={"setInterval","clearInterval","setTimeout","clearTimeout"};
            for(unsigned i=0;i<4;++i) {FVM_Value fn;assert(fvm_get_global(c,names[i],&fn)==FVM_OK);assert(fvm_numeric_callback_id(fn.object->invocable.callback)==164+i);assert(fvm_numeric_callback_from_id(164+i)==fn.object->invocable.callback);}
            if(version && version<6) {
                timer_call(c,"setInterval",2,args,FVM_UNSUPPORTED,route);timer_call(c,"clearInterval",1,args,FVM_UNSUPPORTED,route);
                timer_call(c,"setTimeout",2,args,FVM_UNSUPPORTED,route);timer_call(c,"clearTimeout",1,args,FVM_UNSUPPORTED,route);
                assert(c->scheduled_calls.empty());fvm_context_destroy(c);continue;
            }
            FVM_Value id=timer_call(c,"setInterval",2,args,FVM_OK,route);assert(id.type==FVM_VALUE_NUMBER && id.number==1);
            assert(timer_poll(c,1009)==0 && !probe.calls);assert(timer_poll(c,1010)==1 && probe.calls==1);
            assert(timer_poll(c,2000)==1 && probe.calls==2);assert(timer_poll(c,2000)==0 && probe.calls==2);
            assert(timer_poll(c,1500)==0);timer_call(c,"clearInterval",1,&id,FVM_OK,route);assert(timer_poll(c,3000)==0);
            assert(timer_call(c,"clearInterval",1,&id,FVM_OK,route).type==FVM_VALUE_UNDEFINED);
            timer_call(c,"clearInterval",0,0,FVM_OK,route);
            if(version && version<8)timer_call(c,"setTimeout",2,args,FVM_UNSUPPORTED,route);
            else {
                id=timer_call(c,"setTimeout",2,args,FVM_OK,route);assert(id.number==2);
                assert(timer_poll(c,3010)==1 && probe.calls==3);assert(timer_poll(c,4010)==0 && c->scheduled_calls.empty());
                id=timer_call(c,"setTimeout",2,args,FVM_OK,route);timer_call(c,"clearTimeout",1,&id,FVM_OK,route);assert(timer_poll(c,5010)==0);
            }
            double invalid[]={0,-1,0.5,HUGE_VAL,2147483648.0,std::sqrt(-1.0)};
            for(unsigned i=0;i<sizeof(invalid)/sizeof(invalid[0]);++i) {args[1]=timer_number(invalid[i]);timer_call(c,"setInterval",2,args,FVM_UNSUPPORTED,route);assert(c->scheduled_calls.empty());}
            args[1]=timer_number(10.9);id=timer_call(c,"setInterval",2,args,FVM_OK,route);
            timer_available=0;assert(timer_poll(c,6000,FVM_ERROR)==0 && c->scheduled_calls.size()==1);timer_available=1;
            timer_call(c,"clearInterval",1,&id,FVM_OK,route);timer_available=0;timer_call(c,"setInterval",2,args,FVM_ERROR,route);timer_available=1;
            assert(c->scheduled_calls.empty());fvm_context_destroy(c);
        }
    }
    /* The object/method overload resolves the current method at each delivery and preserves this. */
    FVM_Context *c=timer_context(rt);TimerProbe old_probe,new_probe;FVM_Object *owner=0;assert(fvm_object_create(c,&owner)==FVM_OK);
    FVM_Value old_fn=timer_function(c,&old_probe),new_fn=timer_function(c,&new_probe);assert(fvm_object_set(c,owner,"tick",&old_fn)==FVM_OK);
    FVM_Value args[]={timer_object(owner),timer_number(0),timer_number(5)};assert(fvm_value_string(&args[1],"tick")==FVM_OK);
    FVM_Value id=timer_call(c,"setInterval",3,args);assert(fvm_object_set(c,owner,"tick",&new_fn)==FVM_OK);
    assert(timer_poll(c,1005)==1 && !old_probe.calls && new_probe.calls==1 && new_probe.receiver==owner);
    timer_call(c,"clearTimeout",1,&id);assert(c->scheduled_calls.empty());
    /* IDs do not wrap/reuse after the last representable published identifier. */
    c->last_timer_id=4294967294UL;FVM_Value simple[]={new_fn,timer_number(1)};id=timer_call(c,"setTimeout",2,simple);assert(id.number==4294967295.0);
    timer_call(c,"clearInterval",1,&id);timer_call(c,"setTimeout",2,simple,FVM_ERROR);assert(c->scheduled_calls.empty());
    unsigned count=77;assert(fvm_timers_poll(0,&count)==FVM_INVALID_ARGUMENT && count==77);assert(fvm_timers_poll(c,0)==FVM_INVALID_ARGUMENT);
    c->gc_collecting=true;assert(fvm_timers_poll(c,&count)==FVM_INVALID_ARGUMENT && count==77);c->gc_collecting=false;
    fvm_context_destroy(c);fvm_runtime_destroy(rt);
}
