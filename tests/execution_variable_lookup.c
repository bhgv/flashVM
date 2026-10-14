/* C89 plain binding reads preserve undefined presence and explicit call-receiver policy. */
#include "flashvm/execution.h"
#include "flashvm/property.h"
#include "flashvm/swf.h"
#include <assert.h>
int main(void)
{
 FVM_Config cfg;FVM_Runtime *rt;unsigned version;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(version=4;version<=8;++version){FVM_Context *c;FVM_Object *scope,*target,*receiver,*fallback,*owner,*global;FVM_Value name,value,out;int found;
 unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
 assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
 assert(fvm_object_create(c,&scope)==FVM_OK && fvm_object_create(c,&target)==FVM_OK && fvm_object_create(c,&receiver)==FVM_OK && fvm_object_create(c,&fallback)==FVM_OK);
 fvm_value_number(&value,17);assert(fvm_object_set(c,scope,"local",&value)==FVM_OK);fvm_value_number(&value,23);assert(fvm_object_set(c,target,"target",&value)==FVM_OK);fvm_value_number(&value,31);assert(fvm_set_global(c,"global",&value)==FVM_OK && fvm_set_global(c,"THIS",&value)==FVM_OK);
 fvm_value_string(&name,"local");assert(fvm_exec_variable_lookup(c,scope,target,receiver,fallback,1,&name,&out,&owner,&found)==FVM_OK && found && out.number==17 && owner==fallback);
 fvm_value_string(&name,"target");assert(fvm_exec_variable_lookup(c,scope,target,receiver,fallback,1,&name,&out,&owner,&found)==FVM_OK && found && out.number==23 && owner==target);
 fvm_value_string(&name,"global");assert(fvm_exec_variable_lookup(c,scope,target,receiver,fallback,1,&name,&name,&owner,&found)==FVM_OK && found && name.number==31 && owner==fallback);
 fvm_value_undefined(&value);assert(fvm_object_set(c,scope,"u",&value)==FVM_OK);fvm_value_string(&name,"u");assert(fvm_exec_variable_lookup(c,scope,target,receiver,fallback,1,&name,&out,&owner,&found)==FVM_OK && found && out.type==FVM_VALUE_UNDEFINED);
 fvm_value_string(&name,"THIS");assert(fvm_exec_variable_lookup(c,scope,target,receiver,fallback,1,&name,&out,&owner,&found)==FVM_OK && found && owner==fallback);assert(version==6?(out.type==FVM_VALUE_OBJECT && out.object==receiver):(out.type==FVM_VALUE_NUMBER && out.number==31));
 if(version==5){assert(fvm_exec_variable_lookup(c,scope,target,receiver,fallback,0,&name,&out,&owner,&found)==FVM_OK && found && out.object==receiver);}
 fvm_value_string(&name,"_global");assert(fvm_exec_variable_lookup(c,scope,target,receiver,0,1,&name,&out,&owner,&found)==FVM_OK && found && out.type==FVM_VALUE_OBJECT && owner==out.object);global=out.object;
 fvm_value_string(&name,"missing");assert(fvm_exec_variable_lookup(c,scope,target,receiver,0,1,&name,&out,&owner,&found)==FVM_OK && !found && out.type==FVM_VALUE_UNDEFINED && owner==global);
 fvm_value_number(&name,1);fvm_value_number(&out,99);owner=scope;found=99;assert(fvm_exec_variable_lookup(c,scope,target,receiver,fallback,1,&name,&out,&owner,&found)==FVM_INVALID_ARGUMENT && out.number==99 && owner==scope && found==99);
 fvm_context_destroy(c);}
 fvm_runtime_destroy(rt);return 0;
}
