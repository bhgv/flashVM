/* C89 deletion checks presence without invoking accessors or falling past shadowing. */
#include "flashvm/execution.h"
#include "flashvm/standard.h"
#include "flashvm/property.h"
#include <assert.h>
static FVM_Result forbidden(FVM_Context *c,FVM_Object *o,unsigned n,const FVM_Value *a,FVM_Value *v,void *u)
{ (void)c;(void)o;(void)n;(void)a;(void)v;(void)u;assert(0);return FVM_ERROR; }
int main(void)
{
    FVM_Config cfg;FVM_Runtime *rt;FVM_Context *c;FVM_Object *scope,*object,*prototype,*with;
    FVM_Value name,value,out;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);
    assert(fvm_object_create(c,&scope)==FVM_OK && fvm_object_create(c,&object)==FVM_OK && fvm_object_create(c,&prototype)==FVM_OK);
    fvm_value_number(&value,42);assert(fvm_set_global(c,"shadow",&value)==FVM_OK && fvm_object_set(c,prototype,"shadow",&value)==FVM_OK && fvm_object_set_prototype(c,object,prototype)==FVM_OK);
    fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=object;
    assert(fvm_exec_with_scope(c,scope,&value,&with)==FVM_OK);
    fvm_value_string(&name,"shadow");assert(fvm_exec_scope_delete(c,with,0,&name,&out)==FVM_OK && !out.boolean);
    assert(fvm_get_global(c,"shadow",&out)==FVM_OK && out.number==42);
    assert(fvm_object_define_accessor_rooted(c,object,"accessor",forbidden,0,0,0)==FVM_OK);
    fvm_value_string(&name,"accessor");assert(fvm_exec_scope_delete(c,with,0,&name,&out)==FVM_OK && out.boolean);
    fvm_value_number(&value,17);assert(fvm_object_set_bytes(c,scope,"a\0b",3,&value)==FVM_OK);
    fvm_value_string_bytes(&name,"a\0b",3);assert(fvm_exec_scope_delete(c,with,0,&name,&name)==FVM_OK && name.boolean);
    fvm_value_string(&name,"shadow");assert(fvm_exec_scope_delete(c,scope,0,&name,&out)==FVM_OK && out.boolean);
    fvm_value_number(&value,42);assert(fvm_object_set(c,scope,"fixed",&value)==FVM_OK && fvm_object_set_property_flags(c,scope,"fixed",FVM_PROPERTY_DONT_DELETE,0)==FVM_OK);
    fvm_value_string(&name,"fixed");assert(fvm_exec_scope_delete(c,scope,0,&name,&out)==FVM_OK && !out.boolean);
    fvm_value_string(&name,"missing");assert(fvm_exec_scope_delete(c,scope,0,&name,&out)==FVM_OK && !out.boolean);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
