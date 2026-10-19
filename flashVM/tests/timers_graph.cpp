/* Imported scheduling globals retain trusted collecting coercion but never copy pending queues. */
#include "timers_fixture.hpp"
#include "flashvm/serialization.h"
struct Coercion { unsigned calls;double result;Coercion():calls(0),result(5){} };
static FVM_Result number_body(FVM_Context *c,const FVM_ScriptFrame *,FVM_Value *out,void *raw)
{ Coercion *s=(Coercion *)raw;++s->calls;assert(!c->gc_native_depth && fvm_gc_collect(c,0)==FVM_OK);fvm_value_number(out,s->result);return FVM_OK; }
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    const char *names[]={"setInterval","clearInterval","setTimeout","clearTimeout"};
    for(unsigned index=0;index<4;++index) {
        FVM_Context *a=timer_context(rt);FVM_Value source;assert(fvm_get_global(a,names[index],&source)==FVM_OK);
        FVM_Value callback;assert(fvm_get_global(a,"getTimer",&callback)==FVM_OK);FVM_Value pending[]={callback,timer_number(1)};timer_call(a,"setInterval",2,pending);
        unsigned char *bytes=0;size_t size=0;assert(fvm_value_serialize(a,&source,&bytes,&size)==FVM_OK);fvm_context_destroy(a);
        FVM_Context *b=timer_context(rt);FVM_Value imported;assert(fvm_value_deserialize(b,bytes,size,&imported)==FVM_OK);fvm_serialized_free(bytes);
        assert(b->scheduled_calls.empty() && imported.object->invocable.collector_entry && fvm_gc_pin(b,imported.object)==FVM_OK);
        Coercion state;const unsigned char empty[]={0};FVM_Object *convert=0,*coercible=0;
        assert(fvm_function_create_avm1(b,empty,sizeof(empty),0,0,0,&convert)==FVM_OK && fvm_function_set_body(b,convert,number_body,&state,0)==FVM_OK);
        assert(fvm_object_create(b,&coercible)==FVM_OK);FVM_Value value=timer_object(convert);assert(fvm_object_set(b,coercible,"valueOf",&value)==FVM_OK);
        TimerProbe probe;FVM_Value args[]={timer_function(b,&probe),timer_object(coercible)},out;
        if(index==0 || index==2) {
            assert(fvm_function_invoke(b,imported.object,0,2,args,&out)==FVM_OK && state.calls==1 && out.number==1);
            assert(timer_poll(b,1005)==1 && probe.calls==1);
        } else {
            FVM_Value simple[]={args[0],timer_number(5)};timer_call(b,"setTimeout",2,simple);state.result=1;
            assert(fvm_function_invoke(b,imported.object,0,1,args+1,&out)==FVM_OK && state.calls==1 && b->scheduled_calls.empty());
        }
        fvm_context_destroy(b);
    }
    fvm_runtime_destroy(rt);
}
