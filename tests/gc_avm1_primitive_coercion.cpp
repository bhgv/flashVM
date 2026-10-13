/* Temporary primitive strings remain rooted when another coercion invokes script GC. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include "avm1_standard_types.cpp"
#include "flashvm/extensions.h"
#include "flashvm/gc.h"
static unsigned calls,order[2];
static FVM_Result ordered(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *raw)
{ unsigned id=*(unsigned *)raw;order[calls++]=id;fvm_value_number(out,id);return FVM_OK; }
static void value_of(FVM_Context *c,FVM_Object *owner,Bytes body)
{
    body.push_back(0x3e);body.push_back(0);FVM_Object *fn=0;
    assert(fvm_function_create_avm1(c,body.begin(),body.size(),0,0,0,&fn)==FVM_OK);
    FVM_Value value;fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=fn;
    assert(fvm_object_set(c,owner,"valueOf",&value)==FVM_OK);
}
static FVM_Value operation(FVM_Context *c,unsigned opcode,FVM_Object *left,FVM_Object *right)
{
    object(c,"left",left);object(c,"right",right);Bytes b;global(b,"left");global(b,"right");b.push_back((unsigned char)opcode);return run(c,b);
}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);assert(fvm_context_create(rt,&c)==FVM_OK);
    const unsigned char swf[]={'F','W','S',8,16,0,0,0,0x08,0,0,12,1,0,0,0};assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);assert(fvm_extensions_install(c)==FVM_OK);
    FVM_Object *concat=0,*sweep=0;assert(fvm_construct(c,"Object",0,0,&concat)==FVM_OK);assert(fvm_construct(c,"Object",0,0,&sweep)==FVM_OK);
    Bytes body;text(body,"prefix");text(body,"suffix");body.push_back(0x21);value_of(c,concat,body);
    body.clear();integer(body,0);global(body,"gc");text(body,"collect");body.push_back(0x52);body.push_back(0x17);text(body,"LEFT_LONG_RETURN");value_of(c,sweep,body);
    assert(fvm_gc_string_mode(c,FVM_GC_STRINGS_TRACED)==FVM_OK);
    /* Less2 converts left first; Greater/Add2 convert right first. */
    for(unsigned i=0;i<30;++i)
    {
        FVM_Value result=operation(c,0x47,sweep,concat);assert(result.type==FVM_VALUE_STRING && !std::strcmp(result.string,"LEFT_LONG_RETURNprefixsuffix"));
        assert(!operation(c,0x48,concat,sweep).boolean);assert(operation(c,0x67,concat,sweep).boolean);assert(!operation(c,0x67,sweep,concat).boolean);
    }
    FVM_Object *a=0,*b=0;assert(fvm_construct(c,"Object",0,0,&a)==FVM_OK);assert(fvm_construct(c,"Object",0,0,&b)==FVM_OK);unsigned ids[]={1,2};assert(fvm_object_define_method_rooted(c,a,"valueOf",ordered,ids,0)==FVM_OK);assert(fvm_object_define_method_rooted(c,b,"valueOf",ordered,ids+1,0)==FVM_OK);
    calls=0;assert(operation(c,0x47,a,b).number==3 && calls==2 && order[0]==2 && order[1]==1);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
