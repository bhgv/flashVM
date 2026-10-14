/* Named function and constructor calls share the loaded count conversion policy. */
#define main function2_fixture_main
#include "avm1_function2.cpp"
#undef main
#include "flashvm/execution.h"
static unsigned received;
static FVM_Result body(FVM_Context *,const FVM_ScriptFrame *frame,FVM_Value *out,void *)
{ received=frame->argc;if(received)assert(received==1 && frame->argv[0].number==77);fvm_value_number(out,received);return FVM_OK; }
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=4;version<=8;++version){
        FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);
        unsigned char swf[]={'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
        const unsigned char code[]={0};FVM_Object *callee=0;FVM_Value value,result;
        assert(fvm_function_create_avm1(c,code,sizeof(code),0,0,0,&callee)==FVM_OK && fvm_function_set_body(c,callee,body,0,0)==FVM_OK);
        fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=callee;assert(fvm_set_global(c,"callee",&value)==FVM_OK);
        for(unsigned action=0;action<2;++action){
            Bytes bytes;integer(bytes,77);variable(bytes,"count");text(bytes,"callee");bytes.push_back(action?0x40:0x3d);bytes.push_back(0x3e);bytes.push_back(0);
            FVM_Object *fn=0;assert(fvm_function_create_avm1(c,bytes.begin(),bytes.size(),0,0,0,&fn)==FVM_OK && fvm_gc_pin(c,fn)==FVM_OK);
            fvm_value_string(&value,"9");assert(fvm_set_global(c,"count",&value)==FVM_OK);
            assert(fvm_function_invoke(c,fn,0,0,0,&result)==FVM_OK && received==1);
            assert(action ? result.type==FVM_VALUE_OBJECT : result.number==1);
            fvm_value_number(&value,0.9);assert(fvm_set_global(c,"count",&value)==FVM_OK);
            assert(fvm_function_invoke(c,fn,0,0,0,&result)==FVM_OK && received==0);
            assert(action ? result.type==FVM_VALUE_OBJECT : result.number==0);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
