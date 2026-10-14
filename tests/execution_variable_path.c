/* C89 versioned path phase preserves plain-name fallback and distinguishes undefined presence. */
#include "flashvm/execution.h"
#include "flashvm/property.h"
#include "flashvm/swf.h"
#include <assert.h>
#include <string.h>
int main(void)
{
 FVM_Config cfg;FVM_Runtime *rt;unsigned version;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(version=4;version<=8;++version){FVM_Context *c;FVM_Object *o,*child;FVM_Value value,name,out;int handled,present;
 unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
 assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
 assert(fvm_object_create(c,&o)==FVM_OK && fvm_object_create(c,&child)==FVM_OK);fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=o;assert(fvm_set_global(c,"o",&value)==FVM_OK);value.object=child;assert(fvm_object_set(c,o,"child",&value)==FVM_OK);
 fvm_value_number(&value,17);assert(fvm_object_set(c,o,"member",&value)==FVM_OK && fvm_object_set_bytes(c,o,"a\0b",3,&value)==FVM_OK);fvm_value_undefined(&value);assert(fvm_object_set(c,o,"u",&value)==FVM_OK);
 fvm_value_string(&name,"o.member");fvm_value_number(&out,99);assert(fvm_exec_variable_path(c,0,0,0,&name,&out,&handled,&present)==FVM_OK);
 assert(version>=5?(handled && present && out.number==17):(!handled && !present && out.number==99));
 fvm_value_string(&name,"o:member");assert(fvm_exec_variable_path(c,0,0,0,&name,&name,&handled,&present)==FVM_OK && handled && present && name.number==17);
 fvm_value_string(&name,"o:u");assert(fvm_exec_variable_path(c,0,0,0,&name,&out,&handled,&present)==FVM_OK && handled && present && out.type==FVM_VALUE_UNDEFINED);
 fvm_value_string(&name,"o:missing");assert(fvm_exec_variable_path(c,0,0,0,&name,&out,&handled,&present)==FVM_OK && handled && !present && out.type==FVM_VALUE_UNDEFINED);
 fvm_value_string(&name,":member");assert(fvm_exec_variable_path(c,0,0,0,&name,&out,&handled,&present)==FVM_OK && handled && !present && out.type==FVM_VALUE_UNDEFINED);
 fvm_value_string_bytes(&name,"o:a\0b",5);assert(fvm_exec_variable_path(c,0,0,0,&name,&out,&handled,&present)==FVM_OK && handled && present && out.number==17);
 fvm_value_string(&name,"o/child");fvm_value_number(&out,99);assert(fvm_exec_variable_path(c,0,0,0,&name,&out,&handled,&present)==FVM_OK);assert(version>=5?(handled && present && out.object==child):(!handled && out.number==99));
 fvm_value_string(&name,"o/missing");fvm_value_number(&out,99);assert(fvm_exec_variable_path(c,0,0,0,&name,&out,&handled,&present)==FVM_OK && !handled && !present && out.number==99);
 fvm_value_string(&name,"plain");assert(fvm_exec_variable_path(c,0,0,0,&name,&name,&handled,&present)==FVM_OK && !handled && name.type==FVM_VALUE_STRING && !strcmp(name.string,"plain"));
 if(version>=5){fvm_value_string(&name,"_global.o");assert(fvm_exec_variable_path(c,0,0,0,&name,&out,&handled,&present)==FVM_OK && handled && present && out.object==o);fvm_value_string(&name,"this.member");assert(fvm_exec_variable_path(c,0,0,o,&name,&out,&handled,&present)==FVM_OK && handled && present && out.number==17);}
 fvm_value_number(&name,1);fvm_value_number(&out,99);handled=21;present=22;assert(fvm_exec_variable_path(c,0,0,0,&name,&out,&handled,&present)==FVM_INVALID_ARGUMENT && out.number==99 && handled==21 && present==22);
 fvm_context_destroy(c);}
 fvm_runtime_destroy(rt);return 0;
}
