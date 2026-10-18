/* Loaded method-name coercion, callback roots, construction and bytecode bridge. */
#define main function2_fixture_main
#include "avm1_function2.cpp"
#undef main
#include "flashvm/execution.h"
#include "flashvm/exception.h"
static unsigned invoked, converted;
static bool fail_conversion;
static FVM_Result target(FVM_Context *,FVM_Object *,unsigned n,const FVM_Value *,FVM_Value *out,void *)
{ assert(n==0);++invoked;fvm_value_number(out,42);return FVM_OK; }
static FVM_Result convert(FVM_Context *c,const FVM_ScriptFrame *,FVM_Value *out,void *raw)
{
    ++converted;assert(fvm_gc_collect(c,0)==FVM_OK);
    if(fail_conversion){FVM_Value e;fvm_value_number(&e,77);return fvm_exception_raise(c,&e);}
    return fvm_value_string(out,(const char *)raw);
}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=4;version<=8;++version){
        FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);
        unsigned char swf[]={'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
        FVM_Object *o=0,*fn=0,*key=0,*converter=0;FVM_Value owner,namev,value,out;
        assert(fvm_object_create(c,&o)==FVM_OK && fvm_gc_pin(c,o)==FVM_OK);
        assert(fvm_function_create_native(c,target,0,0,&fn)==FVM_OK);
        fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=fn;
        assert(fvm_object_set(c,o,"17",&value)==FVM_OK && fvm_object_set(c,o,"null",&value)==FVM_OK);
        assert(fvm_object_set(c,o,version==4?"1":"true",&value)==FVM_OK);
        fvm_value_undefined(&owner);owner.type=FVM_VALUE_OBJECT;owner.object=o;
        fvm_value_number(&namev,17);assert(fvm_exec_call_method(c,0,&owner,&namev,0,0,0,&out)==FVM_OK && out.number==42);
        fvm_value_undefined(&namev);namev.type=FVM_VALUE_NULL;
        assert(fvm_exec_call_method(c,0,&owner,&namev,0,0,0,&out)==FVM_OK && out.number==42);
        fvm_value_undefined(&namev);namev.type=FVM_VALUE_BOOLEAN;namev.boolean=1;
        assert(fvm_exec_call_method(c,0,&owner,&namev,0,0,0,&out)==FVM_OK && out.number==42);
        assert(fvm_set_global(c,"receiver",&owner)==FVM_OK);
        Bytes code;text(code,"result");integer(code,0);variable(code,"receiver");integer(code,17);
        code.push_back(0x52);code.push_back(0x1d);code.push_back(0);
        assert(fvm_avm1_execute(c,code.begin(),code.size())==FVM_OK);
        assert(fvm_get_global(c,"result",&out)==FVM_OK && out.number==42);
        assert(fvm_object_create(c,&key)==FVM_OK && fvm_gc_pin(c,key)==FVM_OK);
        const unsigned char body[]={0};
        assert(fvm_function_create_avm1(c,body,sizeof(body),0,0,0,&converter)==FVM_OK);
        assert(fvm_function_set_body(c,converter,convert,(void *)"charAt",0)==FVM_OK);
        value.object=converter;assert(fvm_object_set(c,key,"toString",&value)==FVM_OK);
        fvm_value_string(&owner,"abc");fvm_value_undefined(&namev);namev.type=FVM_VALUE_OBJECT;namev.object=key;
        fvm_value_number(&value,1);
        assert(fvm_exec_call_method(c,0,&owner,&namev,0,1,&value,&out)==FVM_OK && out.type==FVM_VALUE_STRING && out.string[0]=='b');
        fail_conversion=true;fvm_value_number(&out,99);
        assert(fvm_exec_call_method(c,0,&owner,&namev,0,1,&value,&out)==FVM_EXCEPTION && out.number==99);
        assert(fvm_exception_get(c,&out)==FVM_OK && out.number==77 && fvm_exception_clear(c)==FVM_OK);
        fail_conversion=false;
        unsigned before=converted;fvm_value_undefined(&owner);owner.type=FVM_VALUE_NULL;
        assert(fvm_exec_call_method(c,1,&owner,&namev,0,0,0,&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED && converted==before);
        owner.type=FVM_VALUE_OBJECT;owner.object=o;fvm_value_number(&value,3);assert(fvm_object_set(c,o,"plain",&value)==FVM_OK);
        fvm_value_string(&namev,"plain");assert(fvm_exec_call_method(c,1,&owner,&namev,0,0,0,&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
        fvm_context_destroy(c);
    }
    assert(invoked==20 && converted==10);fvm_runtime_destroy(rt);
}
