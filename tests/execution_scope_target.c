/* C89 complete-path retries, retained With scopes and selected-root fallback. */
#include "flashvm/execution.h"
#include "flashvm/property.h"
#include "flashvm/gc.h"
#include <assert.h>
static void object_value(FVM_Value *value,FVM_Object *object)
{ fvm_value_undefined(value);value->type=FVM_VALUE_OBJECT;value->object=object; }
int main(void)
{
 FVM_Config cfg;FVM_Runtime *rt;FVM_Context *c;FVM_Object *inner,*outer,*bad,*good,*leaf,*scope,*target;FVM_Value value,path,out;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
 assert(fvm_object_create(c,&inner)==FVM_OK && fvm_object_create(c,&outer)==FVM_OK && fvm_object_create(c,&bad)==FVM_OK && fvm_object_create(c,&good)==FVM_OK && fvm_object_create(c,&leaf)==FVM_OK && fvm_object_create(c,&target)==FVM_OK);
 object_value(&value,bad);assert(fvm_object_set(c,inner,"first",&value)==FVM_OK);
 object_value(&value,good);assert(fvm_object_set(c,outer,"first",&value)==FVM_OK);
 object_value(&value,leaf);assert(fvm_object_set(c,good,"second",&value)==FVM_OK);
 object_value(&value,inner);assert(fvm_exec_with_scope(c,outer,&value,&scope)==FVM_OK);
 fvm_value_string(&path,"first/second");assert(fvm_exec_scope_target(c,scope,target,0,&path,&out)==FVM_OK && out.object==leaf);
 assert(fvm_exec_scope_target(c,scope,target,0,&path,&path)==FVM_OK && path.object==leaf);
 fvm_value_string(&path,"missing/second");assert(fvm_exec_scope_target(c,scope,target,0,&path,&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
 object_value(&value,leaf);assert(fvm_object_set(c,target,"fallback",&value)==FVM_OK);
 fvm_value_string(&path,"/fallback");assert(fvm_exec_scope_target(c,scope,target,0,&path,&out)==FVM_OK && out.object==leaf);
 fvm_value_string(&path,"_root.fallback");assert(fvm_exec_scope_target(c,scope,target,0,&path,&out)==FVM_OK && out.object==leaf);
 fvm_value_string(&path,"this/second");assert(fvm_exec_scope_target(c,scope,target,good,&path,&out)==FVM_OK && out.object==leaf);
 fvm_value_number(&path,1);fvm_value_number(&out,99);assert(fvm_exec_scope_target(c,scope,target,0,&path,&out)==FVM_INVALID_ARGUMENT && out.number==99);
 fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
