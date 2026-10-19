/* Imported AVM1 tick methods observe sorted depths after sender destruction in interpreter/native JIT. */
#include "clip_actions_fixture.hpp"
#include "movieclip_tick_order_fixture.hpp"
#include "flashvm/execution_mode.h"
#include "flashvm/function.h"
#include "flashvm/jit.h"
/* Shared root decimal trace makes the child callback sequence script-visible. */
static Bytes trace_code(unsigned digit)
{
    Bytes body;variable(body,"_root");text(body,"trace");variable(body,"_root");text(body,"trace");
    body.push_back(0x4e);integer(body,10);body.push_back(0x0c);integer(body,digit);
    body.push_back(0x47);body.push_back(0x4f);body.push_back(0);return body;
}
/* The graph, not any host userdata callback, carries each receiver's AVM1 digit. */
int main()
{
    FVM_Config cfg;FVM_Runtime *runtime=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&runtime)==FVM_OK);
    for(unsigned version=5;version<=8;++version)for(unsigned jit=0;jit<2;++jit)
    {
        if(jit && !fvm_jit_available())continue;
        FVM_Context *source=tick_context(runtime,version),*dest=tick_context(runtime,version);
        FVM_Object *root=tick_root(source);
        FVM_Object *children[]={ordered_child(source,root,"high",10),ordered_child(source,root,"low",1),ordered_child(source,root,"middle",5)};
        const unsigned digits[]={3,1,2};
        for(unsigned i=0;i<3;++i)
        {
            Bytes body=trace_code(digits[i]);FVM_Object *function=0;
            assert(fvm_function_create_avm1(source,body.begin(),body.size(),0,0,0,&function)==FVM_OK);
            FVM_Value value=tick_object(function);assert(fvm_object_set(source,children[i],"onEnterFrame",&value)==FVM_OK);
        }
        FVM_Value value=tick_object(root);unsigned char *wire=0;size_t size=0;
        assert(fvm_value_serialize(source,&value,&wire,&size)==FVM_OK);fvm_context_destroy(source);
        assert(fvm_value_deserialize(dest,wire,size,&value)==FVM_OK);fvm_serialized_free(wire);root=value.object;
        assert(fvm_gc_pin(dest,root)==FVM_OK && fvm_gc_string_mode(dest,FVM_GC_STRINGS_TRACED)==FVM_OK && fvm_gc_collect(dest,0)==FVM_OK);
        fvm_value_number(&value,0);assert(fvm_object_set(dest,root,"trace",&value)==FVM_OK);
        assert(fvm_execution_set_mode(dest,jit?FVM_EXECUTION_JIT:FVM_EXECUTION_INTERPRETER,0)==FVM_OK);
        unsigned long before=dest->execution_mode.stats.native_entries;
        assert(fvm_movieclip_advance(dest,root,1)==FVM_OK);number(dest,root,"trace",123);
        assert((dest->execution_mode.stats.native_entries>before)==(jit!=0));
        tick_clean(dest);fvm_context_destroy(dest);
    }
    fvm_runtime_destroy(runtime);
}
