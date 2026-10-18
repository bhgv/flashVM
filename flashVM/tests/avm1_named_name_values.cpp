/* Named actions convert names before counts, retaining callback-produced names through GC. */
#define main function2_fixture_main
#include "avm1_function2.cpp"
#undef main
#include "flashvm/execution.h"
#include "flashvm/exception.h"
static unsigned order[4],calls;static bool failure;
static FVM_Result name_body(FVM_Context *c,const FVM_ScriptFrame *,FVM_Value *out,void *)
{
    order[calls++]=1;assert(fvm_gc_collect(c,0)==FVM_OK);
    if(failure){FVM_Value e;fvm_value_number(&e,77);return fvm_exception_raise(c,&e);}
    return fvm_value_string(out,"17");
}
static FVM_Result count_body(FVM_Context *c,const FVM_ScriptFrame *,FVM_Value *out,void *)
{ order[calls++]=2;assert(fvm_gc_collect(c,0)==FVM_OK);fvm_value_number(out,0);return FVM_OK; }
static FVM_Result target(FVM_Context *,const FVM_ScriptFrame *frame,FVM_Value *out,void *)
{ order[calls++]=3;assert(frame->argc==0);fvm_value_number(out,42);return FVM_OK; }
static FVM_Object *function(FVM_Context *c,FVM_ScriptBody body)
{ const unsigned char code[]={0};FVM_Object *fn=0;assert(fvm_function_create_avm1(c,code,sizeof(code),0,0,0,&fn)==FVM_OK && fvm_function_set_body(c,fn,body,0,0)==FVM_OK);return fn; }
static FVM_Object *operand(FVM_Context *c,const char *method,FVM_ScriptBody body)
{ FVM_Object *o=0;FVM_Value v;assert(fvm_object_create(c,&o)==FVM_OK);fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=function(c,body);assert(fvm_object_set(c,o,method,&v)==FVM_OK);return o; }
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=4;version<=8;++version){
        FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);
        unsigned char swf[]={'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
        FVM_Value v,out;fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=function(c,target);assert(fvm_set_global(c,"17",&v)==FVM_OK);
        v.object=operand(c,"toString",name_body);assert(fvm_set_global(c,"name",&v)==FVM_OK);
        v.object=operand(c,"valueOf",count_body);assert(fvm_set_global(c,"count",&v)==FVM_OK);
        for(unsigned action=0;action<2;++action){
            Bytes code;variable(code,"count");variable(code,"name");code.push_back(action?0x40:0x3d);code.push_back(0x3e);code.push_back(0);
            FVM_Object *fn=0;assert(fvm_function_create_avm1(c,code.begin(),code.size(),0,0,0,&fn)==FVM_OK && fvm_gc_pin(c,fn)==FVM_OK);
            calls=0;assert(fvm_function_invoke(c,fn,0,0,0,&out)==FVM_OK);
            assert(calls==3 && order[0]==1 && order[1]==2 && order[2]==3);
            assert(action ? out.type==FVM_VALUE_OBJECT : out.number==42);
            failure=true;calls=0;assert(fvm_function_invoke(c,fn,0,0,0,&out)==FVM_EXCEPTION && calls==1 && order[0]==1);
            assert(fvm_exception_clear(c)==FVM_OK);failure=false;
            fvm_value_number(&v,17);assert(fvm_set_global(c,"name",&v)==FVM_OK);calls=0;
            assert(fvm_function_invoke(c,fn,0,0,0,&out)==FVM_OK && calls==2 && order[0]==2 && order[1]==3);
            v.type=FVM_VALUE_OBJECT;v.object=operand(c,"toString",name_body);assert(fvm_set_global(c,"name",&v)==FVM_OK);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
