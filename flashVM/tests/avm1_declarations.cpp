/* Top-level and function declarations retain existing values and complete coerced names. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include "avm1_standard_types.cpp"
static unsigned getter_calls,setter_calls;
static FVM_Result getter(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{ ++getter_calls;fvm_value_number(out,12);return FVM_OK; }
static FVM_Result setter(FVM_Context *,FVM_Object *,unsigned n,const FVM_Value *a,FVM_Value *out,void *)
{ assert(n==1 && a[0].number==9);++setter_calls;fvm_value_undefined(out);return FVM_OK; }
static FVM_Result fail(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *,void *)
{ return FVM_ERROR; }
static void execute(FVM_Context *c,Bytes b,FVM_Object *receiver=0)
{
    b.push_back(0);
    assert((receiver?fvm_avm1_execute_receiver(c,b.begin(),b.size(),receiver):fvm_avm1_execute(c,b.begin(),b.size()))==FVM_OK);
}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK);
    assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);assert(fvm_context_create(rt,&c)==FVM_OK);
    assert(fvm_standard_install(c)==FVM_OK);
    Bytes b;text(b,"v");integer(b,42);b.push_back(0x3c);execute(c,b);
    b.clear();text(b,"v");b.push_back(0x41);execute(c,b);
    FVM_Value value;assert(fvm_get_global(c,"v",&value)==FVM_OK && value.number==42);
    b.clear();text(b,"new");b.push_back(0x41);execute(c,b);
    Bytes global_body;global(global_body,"_global");FVM_Object *globals=run(c,global_body).object;
    int own=0;assert(fvm_object_has_own(c,globals,"new",&own)==FVM_OK && own);
    assert(fvm_value_string_bytes(&value,"p\0x",3)==FVM_OK);assert(fvm_set_global(c,"key",&value)==FVM_OK);
    b.clear();global(b,"key");integer(b,7);b.push_back(0x3c);execute(c,b);
    assert(fvm_get_global_bytes(c,"p\0x",3,&value)==FVM_OK && value.number==7);
    b.clear();integer(b,7);integer(b,8);b.push_back(0x3c);execute(c,b);
    assert(fvm_get_global(c,"7",&value)==FVM_OK && value.number==8);
    FVM_Object *proto=0;assert(fvm_object_create(c,&proto)==FVM_OK);
    assert(fvm_object_define_accessor_rooted(c,proto,"inherited",getter,setter,0,0)==FVM_OK);
    assert(fvm_object_set_prototype(c,globals,proto)==FVM_OK);
    b.clear();text(b,"inherited");b.push_back(0x41);execute(c,b);
    assert(getter_calls==0 && setter_calls==0);
    assert(fvm_object_has_own(c,globals,"inherited",&own)==FVM_OK && !own);
    b.clear();text(b,"inherited");integer(b,9);b.push_back(0x3c);execute(c,b);
    assert(getter_calls==0 && setter_calls==1);
    assert(fvm_object_set_prototype(c,globals,0)==FVM_OK);
    FVM_Object *clip=0;assert(fvm_movieclip_create(c,"root",1,&clip)==FVM_OK);
    b.clear();global(b,"key");integer(b,11);b.push_back(0x3c);execute(c,b,clip);
    assert(fvm_object_get_bytes(c,clip,"p\0x",3,&value)==FVM_OK && value.number==11);
    assert(fvm_get_global_bytes(c,"p\0x",3,&value)==FVM_OK && value.number==7);
    /* An uninitialized local declaration must not erase a formal parameter. */
    b.clear();text(b,"formal");b.push_back(0x41);global(b,"formal");b.push_back(0x3e);b.push_back(0);
    const char *parameters[]={"formal"};FVM_Object *fn=0;
    assert(fvm_function_create_avm1(c,b.begin(),b.size(),1,parameters,0,&fn)==FVM_OK);
    fvm_value_number(&value,123);FVM_Value result;
    assert(fvm_function_invoke(c,fn,0,1,&value,&result)==FVM_OK && result.number==123);
    FVM_Object *name=0;assert(fvm_construct(c,"Object",0,0,&name)==FVM_OK);
    assert(fvm_object_define_method_rooted(c,name,"toString",fail,0,0)==FVM_OK);
    fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=name;
    assert(fvm_set_global(c,"badName",&value)==FVM_OK);
    b.clear();global(b,"badName");b.push_back(0x41);b.push_back(0);
    assert(fvm_avm1_execute(c,b.begin(),b.size())==FVM_ERROR);
    const unsigned char empty[]={0x3c,0};assert(fvm_avm1_execute(c,empty,sizeof(empty))==FVM_ERROR);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
