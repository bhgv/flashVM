/* Call counts are values: conversion, wrapping, shortage and callback failure. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include "execution_value_callbacks.cpp"
#include <limits>
static unsigned received;
static FVM_Result collect_args(FVM_Context *,FVM_Object *,unsigned n,const FVM_Value *args,FVM_Value *out,void *)
{ received=n;if(n)assert(n==1 && args[0].number==77);fvm_value_number(out,n);return FVM_OK; }
static FVM_Result construct_body(FVM_Context *,const FVM_ScriptFrame *frame,FVM_Value *out,void *)
{ received=frame->argc;assert(received==1 && frame->argv[0].number==77);fvm_value_number(out,received);return FVM_OK; }
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=4;version<=8;++version){
        FVM_Context *c=0;FVM_Object *o=0,*fn=0,*count_object=0;FVM_Value value,result;unsigned count=99;
        assert(fvm_context_create(rt,&c)==FVM_OK);
        fvm_value_string(&value,"1");assert(fvm_exec_argument_count(c,&value,1,&count)==FVM_ERROR && count==99);
        unsigned char swf[]={'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
        const double inputs[]={1.9,-1,4294967297.0,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()};
        const unsigned expected[]={1,3,1,0,0};
        for(unsigned i=0;i<5;++i){fvm_value_number(&value,inputs[i]);assert(fvm_exec_argument_count(c,&value,3,&count)==FVM_OK && count==expected[i]);}
        fvm_value_string(&value,"2");assert(fvm_exec_argument_count(c,&value,1,&count)==FVM_OK && count==1);
        fvm_value_number(&value,-1);count=99;
        assert(fvm_exec_argument_count(c,&value,FVM_EXEC_MAX_METHOD_ARGUMENTS+1U,&count)==FVM_ERROR && count==99);
        assert(fvm_object_create(c,&o)==FVM_OK);
        assert(fvm_function_create_native(c,collect_args,0,0,&fn)==FVM_OK);
        fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=fn;assert(fvm_object_set(c,o,"m",&value)==FVM_OK);
        value.object=o;assert(fvm_set_global(c,"receiver",&value)==FVM_OK);
        Bytes body;integer(body,77);variable(body,"count");variable(body,"receiver");text(body,"m");body.push_back(0x52);body.push_back(0x3e);body.push_back(0);
        assert(fvm_function_create_avm1(c,body.begin(),body.size(),0,0,0,&fn)==FVM_OK && fvm_gc_pin(c,fn)==FVM_OK);
        fvm_value_string(&value,"9");assert(fvm_set_global(c,"count",&value)==FVM_OK);
        assert(fvm_function_invoke(c,fn,0,0,0,&result)==FVM_OK && result.number==1 && received==1);
        fvm_value_number(&value,0.9);assert(fvm_set_global(c,"count",&value)==FVM_OK);
        assert(fvm_function_invoke(c,fn,0,0,0,&result)==FVM_OK && result.number==0 && received==0);
        attach(c,o,"m",construct_body,0);
        body[body.size()-3]=0x53;
        assert(fvm_function_create_avm1(c,body.begin(),body.size(),0,0,0,&fn)==FVM_OK);
        fvm_value_string(&value,"9");assert(fvm_set_global(c,"count",&value)==FVM_OK);
        assert(fvm_function_invoke(c,fn,0,0,0,&result)==FVM_OK && result.type==FVM_VALUE_OBJECT && received==1);
        assert(fvm_object_create(c,&count_object)==FVM_OK && fvm_gc_pin(c,count_object)==FVM_OK);
        unsigned id=2;attach(c,count_object,"valueOf",numeric,&id);
        fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=count_object;calls=0;
        assert(fvm_exec_argument_count(c,&value,1,&count)==FVM_OK && count==1 && calls==1);
        failing=true;calls=0;count=99;
        assert(fvm_exec_argument_count(c,&value,1,&count)==FVM_EXCEPTION && count==99 && calls==1);
        assert(fvm_exception_clear(c)==FVM_OK);failing=false;
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
