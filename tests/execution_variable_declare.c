/* C89 declarations distinguish With own/inherited bindings and local versus top-level paths. */
#include "flashvm/execution.h"
#include "flashvm/property.h"
#include "flashvm/standard.h"
#include <assert.h>
static FVM_Result forbidden(FVM_Context *c,FVM_Object *o,unsigned n,const FVM_Value *a,FVM_Value *out,void *raw)
{ (void)c;(void)o;(void)n;(void)a;(void)out;(void)raw;assert(0);return FVM_ERROR; }
static void object_value(FVM_Value *value,FVM_Object *object)
{ fvm_value_undefined(value);value->type=FVM_VALUE_OBJECT;value->object=object; }
int main(void)
{
 FVM_Config cfg;FVM_Runtime *rt;unsigned version;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(version=4;version<=8;++version){FVM_Context *c;FVM_Object *frame,*outer,*wrapped,*prototype,*scope,*holder,*parent;FVM_Value name,value,out;int own;
 unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
 assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
 assert(fvm_object_create(c,&frame)==FVM_OK && fvm_object_create(c,&outer)==FVM_OK && fvm_object_create(c,&wrapped)==FVM_OK && fvm_object_create(c,&prototype)==FVM_OK && fvm_object_create(c,&holder)==FVM_OK && fvm_object_set_prototype(c,frame,outer)==FVM_OK && fvm_object_set_prototype(c,wrapped,prototype)==FVM_OK);
 object_value(&value,wrapped);assert(fvm_exec_with_scope(c,frame,&value,&scope)==FVM_OK);fvm_value_number(&value,31);assert(fvm_object_set(c,prototype,"x",&value)==FVM_OK);
 fvm_value_number(&value,42);fvm_value_string(&name,"x");assert(fvm_exec_variable_declare(c,scope,frame,0,0,FVM_EXEC_DECLARE_INITIALIZED,&name,&value)==FVM_OK && fvm_object_get(c,frame,"x",&out)==FVM_OK && out.number==42 && fvm_object_has_own(c,wrapped,"x",&own)==FVM_OK && !own);
 fvm_value_number(&out,23);assert(fvm_object_set(c,wrapped,"x",&out)==FVM_OK);fvm_value_number(&value,99);assert(fvm_exec_variable_declare(c,scope,frame,0,0,FVM_EXEC_DECLARE_INITIALIZED,&name,&value)==FVM_OK && fvm_object_get(c,wrapped,"x",&out)==FVM_OK && out.number==99 && fvm_object_get(c,frame,"x",&out)==FVM_OK && out.number==42);
 assert(fvm_exec_variable_declare(c,scope,frame,0,0,FVM_EXEC_DECLARE_UNINITIALIZED,&name,0)==FVM_OK && fvm_object_get(c,wrapped,"x",&out)==FVM_OK && out.number==99);
 fvm_value_number(&value,17);assert(fvm_object_set(c,frame,"z",&value)==FVM_OK);fvm_value_string(&name,"z");assert(fvm_exec_variable_declare(c,scope,frame,0,0,FVM_EXEC_DECLARE_UNINITIALIZED,&name,0)==FVM_OK && fvm_object_get(c,frame,"z",&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
 assert(fvm_object_define_accessor_rooted(c,outer,"trap",forbidden,forbidden,0,0)==FVM_OK);fvm_value_string(&name,"trap");assert(fvm_exec_variable_declare(c,frame,frame,0,0,FVM_EXEC_DECLARE_INITIALIZED,&name,&value)==FVM_OK && fvm_object_get(c,frame,"trap",&out)==FVM_OK && out.number==17);
 fvm_value_string(&name,"__proto__");assert(fvm_exec_variable_declare(c,frame,frame,0,0,FVM_EXEC_DECLARE_INITIALIZED,&name,&value)==FVM_OK && fvm_object_get_prototype(c,frame,&parent)==FVM_OK && parent==outer && fvm_object_get(c,frame,"__proto__",&out)==FVM_OK && out.number==17);
 object_value(&out,holder);assert(fvm_set_global(c,"holder",&out)==FVM_OK);fvm_value_string(&name,"holder:key");assert(fvm_exec_variable_declare(c,0,0,0,0,FVM_EXEC_DECLARE_UNINITIALIZED,&name,0)==FVM_OK && fvm_object_has_own(c,holder,"key",&own)==FVM_OK && own && fvm_object_get(c,holder,"key",&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
 assert(fvm_exec_variable_declare(c,0,0,0,0,FVM_EXEC_DECLARE_INITIALIZED,&name,&value)==FVM_OK && fvm_object_get(c,holder,"key",&out)==FVM_OK && out.number==17);
 fvm_value_number(&value,42);assert(fvm_exec_variable_declare(c,frame,frame,0,0,FVM_EXEC_DECLARE_INITIALIZED,&name,&value)==FVM_OK && fvm_object_get(c,frame,"holder:key",&out)==FVM_OK && out.number==42 && fvm_object_get(c,holder,"key",&out)==FVM_OK && out.number==17);
 fvm_value_string(&name,"");assert(fvm_exec_variable_declare(c,frame,frame,0,0,FVM_EXEC_DECLARE_INITIALIZED,&name,&value)==FVM_OK && fvm_object_has_own(c,frame,"",&own)==FVM_OK && !own);
 fvm_context_destroy(c);}
 fvm_runtime_destroy(rt);return 0;
}
