/* C89 consumers use semantic operations without Action IDs or operand stacks. */
#include "flashvm/execution.h"
#include "flashvm/standard.h"
#include "flashvm/property.h"
#include <assert.h>
int main(void)
{
    FVM_Config cfg;FVM_Runtime *rt;unsigned version;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(version=4;version<=8;++version){
        FVM_Context *c;FVM_Object *o,*p;FVM_Value a,b,out,owner,key;
        unsigned char swf[]={'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
        fvm_value_string(&a,"0");
        assert(fvm_exec_logical(c,FVM_EXEC_LOGICAL_NOT,&a,0,&a)==FVM_OK && a.type==FVM_VALUE_BOOLEAN && a.boolean==(version<7));
        fvm_value_number(&a,3);fvm_value_string(&b,"3");
        assert(fvm_exec_legacy_compare(c,FVM_EXEC_LEGACY_EQUAL,&a,&b,&b)==FVM_OK && b.boolean);
        fvm_value_number(&b,4);
        assert(fvm_exec_legacy_compare(c,FVM_EXEC_LEGACY_LESS,&a,&b,&a)==FVM_OK && a.boolean);
        fvm_value_number(&out,99);
        assert(fvm_exec_legacy_compare(c,(FVM_ExecutionLegacyCompare)99,&a,&b,&out)==FVM_INVALID_ARGUMENT && out.number==99);
        assert(fvm_exec_logical(c,FVM_EXEC_LOGICAL_AND,&a,0,&out)==FVM_INVALID_ARGUMENT && out.number==99);
        fvm_value_boolean(&a,0);fvm_value_boolean(&b,1);
        assert(fvm_exec_logical(c,FVM_EXEC_LOGICAL_AND,&a,&b,&out)==FVM_OK && !out.boolean);
        assert(fvm_exec_logical(c,FVM_EXEC_LOGICAL_OR,&a,&b,&out)==FVM_OK && out.boolean);
        assert(fvm_object_create(c,&o)==FVM_OK && fvm_object_create(c,&p)==FVM_OK && fvm_object_set_prototype(c,o,p)==FVM_OK);
        fvm_value_number(&a,17);assert(fvm_object_set_bytes(c,p,"a\0b",3,&a)==FVM_OK);
        fvm_value_undefined(&owner);owner.type=FVM_VALUE_OBJECT;owner.object=o;fvm_value_string_bytes(&key,"a\0b",3);
        assert(fvm_exec_delete_member(c,&owner,&key,&out)==FVM_OK && !out.boolean);
        assert(fvm_object_set_bytes(c,o,"a\0b",3,&a)==FVM_OK);
        assert(fvm_exec_delete_member(c,&owner,&key,&key)==FVM_OK && key.boolean);
        fvm_value_string(&key,"fixed");assert(fvm_object_set(c,o,"fixed",&a)==FVM_OK);
        assert(fvm_object_set_property_flags(c,o,"fixed",FVM_PROPERTY_DONT_DELETE,0)==FVM_OK);
        assert(fvm_exec_delete_member(c,&owner,&key,&out)==FVM_OK && !out.boolean);
        assert(fvm_exec_delete_member(c,0,&key,&out)==FVM_INVALID_ARGUMENT);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
