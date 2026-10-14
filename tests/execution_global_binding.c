/* Namespace fallback obeys SWF6 availability and follows ordinary bindings, including undefined. */
#include "flashvm/execution.h"
#include "flashvm/property.h"
#include <assert.h>
int main(void)
{
 FVM_Config cfg;FVM_Runtime *rt;unsigned version;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(version=0;version<=8;++version){FVM_Context *c;FVM_Object *target,*scope,*owner,*global;FVM_Value key,value,out;int found,removed;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};int available=!version || version>=6;if(version && version<4)continue;assert(fvm_context_create(rt,&c)==FVM_OK);if(version){swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);}assert(fvm_object_create(c,&target)==FVM_OK && fvm_object_create(c,&scope)==FVM_OK);
 fvm_value_string(&key,"_global");assert(fvm_exec_variable_lookup(c,0,target,0,0,0,&key,&out,&owner,&found)==FVM_OK && found==available && out.type==(available?FVM_VALUE_OBJECT:FVM_VALUE_UNDEFINED));global=owner;if(available)assert(out.object==global);
 fvm_value_string(&key,"_GLOBAL");assert(fvm_exec_variable_lookup(c,0,target,0,0,0,&key,&out,&owner,&found)==FVM_OK && found==(version==6));
 if(!version){fvm_value_number(&value,31);assert(fvm_set_global(c,"_global",&value)==FVM_OK && fvm_object_set(c,target,"_global",&value)==FVM_OK);fvm_value_string(&key,"_global");assert(fvm_exec_variable_lookup(c,0,target,0,0,0,&key,&out,&owner,&found)==FVM_OK && found && out.type==FVM_VALUE_OBJECT && out.object==global);fvm_value_number(&value,55);assert(fvm_object_set(c,scope,"_global",&value)==FVM_OK && fvm_exec_variable_lookup(c,scope,target,0,0,1,&key,&out,&owner,&found)==FVM_OK && found && out.number==55);fvm_context_destroy(c);continue;}
 fvm_value_number(&value,31);assert(fvm_set_global(c,"_global",&value)==FVM_OK);fvm_value_string(&key,"_global");assert(fvm_exec_variable_lookup(c,0,target,0,0,0,&key,&out,&owner,&found)==FVM_OK && found && out.number==31);
 fvm_value_number(&value,41);assert(fvm_object_set(c,target,"_global",&value)==FVM_OK && fvm_exec_variable_lookup(c,0,target,0,0,0,&key,&out,&owner,&found)==FVM_OK && found && out.number==41 && owner==target);
 fvm_value_undefined(&value);assert(fvm_object_set(c,target,"_global",&value)==FVM_OK && fvm_exec_variable_lookup(c,0,target,0,0,0,&key,&out,&owner,&found)==FVM_OK && found && out.type==FVM_VALUE_UNDEFINED && owner==target);
 assert(fvm_object_delete(c,target,"_global",&removed)==FVM_OK && removed && fvm_set_global(c,"_global",&value)==FVM_OK && fvm_exec_variable_lookup(c,0,target,0,0,0,&key,&out,&owner,&found)==FVM_OK && found && out.type==FVM_VALUE_UNDEFINED);
 assert(fvm_object_delete(c,global,"_global",&removed)==FVM_OK && removed && fvm_exec_variable_lookup(c,0,target,0,0,0,&key,&out,&owner,&found)==FVM_OK && found==available);
 fvm_value_number(&value,55);assert(fvm_object_set(c,scope,"_global",&value)==FVM_OK && fvm_exec_variable_lookup(c,scope,target,0,0,1,&key,&out,&owner,&found)==FVM_OK && found && out.number==55);
 fvm_value_string_bytes(&key,"_global\0x",9);assert(fvm_exec_variable_lookup(c,0,target,0,0,0,&key,&out,&owner,&found)==FVM_OK && !found && out.type==FVM_VALUE_UNDEFINED);
 fvm_context_destroy(c);}
 fvm_runtime_destroy(rt);return 0;
}
