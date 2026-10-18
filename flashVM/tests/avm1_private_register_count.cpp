/* Loaded Function2 counts include slot zero, and private misses do not use globals. */
#define main function2_fixture_main
#include "avm1_function2.cpp"
#undef main
#include "flashvm/execution.h"
#include "../src/core/runtime.hpp"
static FVM_Result probe(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *raw)
{
    unsigned count=*(unsigned *)raw;
    assert(frame->register_count==(count?count:FVM_EXEC_GLOBAL_REGISTER_COUNT));
    if(count)assert(frame->registers!=c->avm1_global_registers);
    else assert(frame->registers==c->avm1_global_registers);
    fvm_value_number(out,frame->register_count);return FVM_OK;
}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=4;version<=8;++version) {
        FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);
        unsigned char swf[]={'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
        FVM_Value value,out;fvm_value_number(&value,73);assert(fvm_exec_global_register(c,1,1,&value,&out)==FVM_OK);
        const unsigned counts[]={1,2,255};
        for(unsigned i=0;i<3;++i) {
            unsigned count=counts[i];const char *params[]={"x"};unsigned dst[]={count-1};
            Bytes body;reg(body,count-1);end(body);FVM_Object *fn=define(c,count,0,dst[0]?1:0,params,dst,body);
            fvm_value_number(&value,42);assert(fvm_function_invoke(c,fn,0,1,&value,&out)==FVM_OK);
            if(dst[0])assert(out.type==FVM_VALUE_NUMBER && out.number==42);else assert(out.type==FVM_VALUE_UNDEFINED);
            Bytes miss;reg(miss,count);end(miss);dst[0]=count;fn=define(c,count,0,1,params,dst,miss);
            assert(fvm_function_invoke(c,fn,0,1,&value,&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
            assert(fvm_function_set_body(c,fn,probe,&count,0)==FVM_OK && fvm_function_invoke(c,fn,0,0,0,&out)==FVM_OK && out.number==count);
            assert(fvm_function_set_body(c,fn,0,0,0)==FVM_OK);
        }
        unsigned count=0;Bytes global;reg(global,1);end(global);FVM_Object *fn=define(c,0,0,0,0,0,global);
        assert(fvm_function_invoke(c,fn,0,0,0,&out)==FVM_OK && out.number==73);
        assert(fvm_function_set_body(c,fn,probe,&count,0)==FVM_OK && fvm_function_invoke(c,fn,0,0,0,&out)==FVM_OK && out.number==FVM_EXEC_GLOBAL_REGISTER_COUNT);
        assert(!c->gc_frames && !c->script_depth);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
