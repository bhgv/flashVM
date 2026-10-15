/* StoreRegister materializes undefined for loaded empty stacks before bank selection. */
#define main function2_fixture_main
#include "avm1_function2.cpp"
#undef main
#include "flashvm/execution.h"
#include "../src/core/runtime.hpp"
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned profile=0;profile<6;++profile)
    {
        FVM_Context *c=0;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)(profile+3);
        assert(fvm_context_create(rt,&c)==FVM_OK);if(profile)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
        const unsigned slots[]={0,1,3,4,255};FVM_Value value,out;
        for(unsigned i=0;i<5;++i)
        {
            fvm_value_number(&value,73);assert(fvm_exec_global_register(c,1,1,&value,&out)==FVM_OK);
            if(slots[i]<4)assert(fvm_exec_global_register(c,1,slots[i],&value,&out)==FVM_OK);
            const unsigned char malformed[]={0x87,0,0,0};assert(fvm_avm1_execute(c,malformed,sizeof(malformed))==FVM_INVALID_ARGUMENT);
            assert(fvm_exec_global_register(c,0,1,0,&out)==FVM_OK && out.number==73);
            Bytes code;code.push_back(0x87);u16(code,1);code.push_back((unsigned char)slots[i]);code.push_back(0);
            FVM_GCRootFrame *roots=c->gc_frames;FVM_Result r=fvm_avm1_execute(c,code.begin(),code.size());assert(c->gc_frames==roots && !c->avm1_execution_depth && !c->script_depth);
            assert(r==(profile?FVM_OK:FVM_ERROR));
            assert(fvm_exec_global_register(c,0,slots[i],0,&out)==FVM_OK);
            if(profile || slots[i]>=4)assert(out.type==FVM_VALUE_UNDEFINED);else assert(out.number==73);
            if(slots[i]!=1)assert(fvm_exec_global_register(c,0,1,0,&out)==FVM_OK && out.number==73);
            if(profile)
            {
                /* Seed private register 1 through a parameter, then overwrite it from empty stack. */
                const char *names[]={"x"};unsigned destinations[]={1};Bytes body;body.push_back(0x87);u16(body,1);body.push_back(1);body.push_back(0x17);reg(body,1);end(body);
                FVM_Object *fn=define(c,2,0,1,names,destinations,body);fvm_value_number(&value,42);
                assert(fvm_function_invoke(c,fn,0,1,&value,&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
                assert(fvm_gc_collect(c,0)==FVM_OK);
            }
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
