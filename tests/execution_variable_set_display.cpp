/* Qualified writes use one owner-path parse, including dotted clip names after a slash. */
#define main paths_fixture_main
#include "movieclip_paths.cpp"
#undef main
#include "flashvm/execution.h"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=4;version<=8;++version){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
 FVM_Object *root=0;assert(fvm_movieclip_create(c,"root",1,&root)==FVM_OK);FVM_Object *a=child(c,root,"a",1),*dot=child(c,a,"x.y",2);FVM_Value name,value,out;int handled,present;fvm_value_number(&value,42);fvm_value_string(&name,"a/x.y:value");
 assert(fvm_exec_variable_set(c,0,root,root,&name,&value)==FVM_OK && fvm_object_get(c,dot,"value",&out)==FVM_OK && out.number==42);
 assert(fvm_exec_variable_path(c,0,root,root,&name,&out,&handled,&present)==FVM_OK && handled && present && out.number==42);
 fvm_value_string(&name,"a/x.y:literal/slash");assert(fvm_exec_variable_set(c,0,root,root,&name,&value)==FVM_OK && fvm_object_get(c,dot,"literal/slash",&out)==FVM_OK && out.number==42);
 fvm_context_destroy(c);}
 fvm_runtime_destroy(rt);return 0;
}
