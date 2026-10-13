/* ActionDelete uses counted canonical keys and own-property deletion rules. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include "avm1_standard_types.cpp"
#include "flashvm/property.h"
#include "flashvm/table.h"
#include "flashvm/gc.h"
static unsigned calls;
static FVM_Result getter(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{ ++calls;fvm_value_number(out,7);return FVM_OK; }
static FVM_Result key_string(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{ ++calls;assert(fvm_gc_collect(c,0)==FVM_INVALID_ARGUMENT);return fvm_value_string_bytes(out,"p\0x",3); }
static FVM_Result fail(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *,void *)
{ return FVM_ERROR; }
static FVM_Value remove(FVM_Context *c,const FVM_Value &owner,const FVM_Value &key)
{
    assert(fvm_set_global(c,"owner",&owner)==FVM_OK);
    assert(fvm_set_global(c,"key",&key)==FVM_OK);
    Bytes b;global(b,"owner");global(b,"key");b.push_back(0x3a);return run(c,b);
}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Object *o=0,*p=0,*name=0;
    assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
    assert(fvm_construct(c,"Object",0,0,&o)==FVM_OK);assert(fvm_construct(c,"Object",0,0,&p)==FVM_OK);
    assert(fvm_object_set_prototype(c,o,p)==FVM_OK);
    FVM_Value owner,key,value;fvm_value_undefined(&owner);owner.type=FVM_VALUE_OBJECT;owner.object=o;
    fvm_value_number(&value,7);assert(fvm_object_set(c,p,"inherited",&value)==FVM_OK);
    fvm_value_string(&key,"inherited");assert(!remove(c,owner,key).boolean);
    assert(fvm_object_get(c,o,"inherited",&value)==FVM_OK && value.number==7);
    fvm_value_string(&key,"absent");assert(!remove(c,owner,key).boolean);
    assert(fvm_object_set_bytes(c,o,"p\0x",3,&value)==FVM_OK);
    assert(fvm_object_set(c,o,"p",&value)==FVM_OK);
    assert(fvm_value_string_bytes(&key,"p\0x",3)==FVM_OK);
    assert(remove(c,owner,key).boolean);assert(!remove(c,owner,key).boolean);
    assert(fvm_object_get(c,o,"p",&value)==FVM_OK && value.number==7);
    assert(fvm_object_set(c,o,"fixed",&value)==FVM_OK);
    assert(fvm_object_set_property_flags(c,o,"fixed",FVM_PROPERTY_DONT_DELETE,0)==FVM_OK);
    fvm_value_string(&key,"fixed");assert(!remove(c,owner,key).boolean);
    assert(fvm_object_define_accessor_rooted(c,o,"accessor",getter,0,0,0)==FVM_OK);
    calls=0;fvm_value_string(&key,"accessor");assert(remove(c,owner,key).boolean && calls==0);
    const char pair[]={char(0xed),char(0xa0),char(0xbd),char(0xed),char(0xb8),char(0x80)};
    assert(fvm_object_set_bytes(c,o,"\xf0\x9f\x98\x80",4,&value)==FVM_OK);
    assert(fvm_value_string_bytes(&key,pair,sizeof(pair))==FVM_OK);assert(remove(c,owner,key).boolean);
    assert(fvm_construct(c,"Array",0,0,&o)==FVM_OK);owner.object=o;
    assert(fvm_table_set_index(c,o,5,&value)==FVM_OK);fvm_value_number(&key,5);
    assert(remove(c,owner,key).boolean);assert(fvm_object_get(c,o,"length",&value)==FVM_OK && value.number==6);
    fvm_value_string(&key,"length");assert(!remove(c,owner,key).boolean);
    assert(fvm_construct(c,"Object",0,0,&name)==FVM_OK);
    assert(fvm_object_define_method_rooted(c,name,"toString",key_string,0,0)==FVM_OK);
    fvm_value_undefined(&key);key.type=FVM_VALUE_OBJECT;key.object=name;
    assert(fvm_object_set_bytes(c,o,"p\0x",3,&value)==FVM_OK);
    calls=0;assert(remove(c,owner,key).boolean && calls==1);
    assert(fvm_object_define_method_rooted(c,name,"toString",fail,0,0)==FVM_OK);
    assert(fvm_set_global(c,"key",&key)==FVM_OK);Bytes bad;global(bad,"owner");global(bad,"key");bad.push_back(0x3a);bad.push_back(0);
    assert(fvm_avm1_execute(c,bad.begin(),bad.size())==FVM_ERROR);
    fvm_value_number(&owner,1);fvm_value_string(&key,"x");assert(!remove(c,owner,key).boolean);
    const unsigned char empty[]={0x3a,0};assert(fvm_avm1_execute(c,empty,sizeof(empty))==FVM_ERROR);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
