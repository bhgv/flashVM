/* Shared services retain operands while prepared native coercions collect and throw. */
#define main function2_fixture_main
#include "avm1_function2.cpp"
#undef main
#include "flashvm/execution.h"
#include "flashvm/exception.h"
static unsigned order[4],calls;
static bool failing;
static FVM_Result numeric(FVM_Context *c,const FVM_ScriptFrame *,FVM_Value *out,void *raw)
{
    unsigned id=*(unsigned *)raw;assert(calls<4);order[calls++]=id;
    assert(fvm_gc_collect(c,0)==FVM_OK);
    if(failing && id==2){FVM_Value e;fvm_value_number(&e,77);return fvm_exception_raise(c,&e);}
    fvm_value_number(out,id);return FVM_OK;
}
static FVM_Result text_key(FVM_Context *c,const FVM_ScriptFrame *,FVM_Value *out,void *)
{ assert(fvm_gc_collect(c,0)==FVM_OK);return fvm_value_string(out,"victim"); }
static void attach(FVM_Context *c,FVM_Object *o,const char *key,FVM_ScriptBody body,void *raw)
{
    const unsigned char code[]={0};FVM_Object *fn=0;FVM_Value value;
    assert(fvm_function_create_avm1(c,code,sizeof(code),0,0,0,&fn)==FVM_OK);
    assert(fvm_function_set_body(c,fn,body,raw,0)==FVM_OK);
    fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=fn;
    assert(fvm_object_set(c,o,key,&value)==FVM_OK);
}
#ifndef FVM_VALUE_CALLBACKS_FIXTURE_ONLY
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Object *a=0,*b=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    unsigned char swf[]={'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};
    assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
    assert(fvm_object_create(c,&a)==FVM_OK && fvm_object_create(c,&b)==FVM_OK);
    assert(fvm_gc_pin(c,a)==FVM_OK && fvm_gc_pin(c,b)==FVM_OK);
    unsigned ids[]={1,2};attach(c,a,"valueOf",numeric,ids);attach(c,b,"valueOf",numeric,ids+1);
    FVM_Value left,right,out;fvm_value_undefined(&left);left.type=FVM_VALUE_OBJECT;left.object=a;right=left;right.object=b;
    assert(fvm_exec_legacy_compare(c,FVM_EXEC_LEGACY_EQUAL,&left,&right,&out)==FVM_OK && !out.boolean);
    assert(calls==2 && order[0]==2 && order[1]==1);calls=0;
    assert(fvm_exec_legacy_compare(c,FVM_EXEC_LEGACY_LESS,&left,&right,&out)==FVM_OK && out.boolean);
    assert(calls==2 && order[0]==1 && order[1]==2);calls=0;failing=true;fvm_value_number(&out,99);
    assert(fvm_exec_legacy_compare(c,FVM_EXEC_LEGACY_EQUAL,&left,&right,&out)==FVM_EXCEPTION && out.number==99);
    assert(calls==1 && order[0]==2 && fvm_exception_clear(c)==FVM_OK);failing=false;
    attach(c,b,"toString",text_key,0);fvm_value_number(&out,17);assert(fvm_object_set(c,a,"victim",&out)==FVM_OK);
    assert(fvm_gc_unpin(c,a)==FVM_OK && fvm_gc_unpin(c,b)==FVM_OK);
    assert(fvm_exec_delete_member(c,&left,&right,&out)==FVM_OK && out.boolean);
    assert(fvm_object_get(c,a,"victim",&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);
}

#endif
