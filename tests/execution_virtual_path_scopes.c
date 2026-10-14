/* Virtual path presence selects With writes/deletes and DefineLocal2, but is not own presence. */
#include "flashvm/execution.h"
#include "flashvm/standard.h"
#include "flashvm/movieclip.h"
#include "flashvm/property.h"
#include <assert.h>
int main(void)
{
 FVM_Config cfg;FVM_Runtime *rt;unsigned version;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(version=4;version<=8;++version){FVM_Context *c;FVM_Object *root,*child,*derived,*activation,*scope,*object;FVM_Value args[2],value,key,out;unsigned form,i;int own,removed;const char *names[]={"_root","_parent","_global"};unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"root",1,&root)==FVM_OK && fvm_object_create(c,&activation)==FVM_OK && fvm_object_create(c,&derived)==FVM_OK);fvm_value_string(args,"child");fvm_value_number(args+1,1);assert(fvm_object_call(c,root,"createEmptyMovieClip",2,args,&out)==FVM_OK);child=out.object;assert(fvm_object_set_prototype(c,derived,child)==FVM_OK);
 for(form=0;form<2;++form){object=form?derived:child;fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=object;assert(fvm_exec_with_scope(c,activation,&value,&scope)==FVM_OK);
 for(i=0;i<3;++i){int intrinsic=version>=(i==2?6U:5U);fvm_value_string(&key,names[i]);fvm_value_number(&value,31);assert(fvm_object_set(c,root,names[i],&value)==FVM_OK);fvm_value_number(&value,99);assert(fvm_object_set(c,activation,names[i],&value)==FVM_OK);fvm_value_number(&value,17);assert(fvm_exec_variable_set(c,scope,root,object,&key,&value)==FVM_OK && fvm_object_has_own(c,object,names[i],&own)==FVM_OK && own==intrinsic);assert(fvm_object_get(c,activation,names[i],&out)==FVM_OK && out.number==(intrinsic?99:17));if(intrinsic)assert(fvm_object_delete(c,object,names[i],&removed)==FVM_OK && removed);
 fvm_value_number(&value,99);assert(fvm_object_set(c,activation,names[i],&value)==FVM_OK && fvm_exec_scope_delete(c,scope,root,&key,&out)==FVM_OK && out.type==FVM_VALUE_BOOLEAN && out.boolean==!intrinsic && fvm_object_has_own(c,activation,names[i],&own)==FVM_OK && own==intrinsic);assert(fvm_object_get(c,root,names[i],&out)==FVM_OK && out.number==31);
 fvm_value_number(&value,99);assert(fvm_object_set(c,activation,names[i],&value)==FVM_OK && fvm_exec_variable_declare(c,scope,activation,root,object,FVM_EXEC_DECLARE_UNINITIALIZED,&key,0)==FVM_OK && fvm_object_get(c,activation,names[i],&out)==FVM_OK);assert(intrinsic?(out.type==FVM_VALUE_NUMBER && out.number==99):out.type==FVM_VALUE_UNDEFINED);
 fvm_value_number(&value,17);assert(fvm_exec_variable_declare(c,scope,activation,root,object,FVM_EXEC_DECLARE_INITIALIZED,&key,&value)==FVM_OK && fvm_object_get(c,activation,names[i],&out)==FVM_OK && out.number==17 && fvm_object_has_own(c,object,names[i],&own)==FVM_OK && !own);}}
 fvm_context_destroy(c);}fvm_runtime_destroy(rt);return 0;
}
