/* Variable-path flags survive exceptions and presence survives final undefined values under GC. */
#define main delete_path_fixture_main
#include "execution_delete_path.c"
#undef main
int main(void)
{
 FVM_Config cfg;FVM_Runtime *rt;FVM_Context *c;FVM_Object *scope,*child;FVM_Value name,value,out;int handled,present;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);
 assert(fvm_construct(c,"Object",0,0,&scope)==FVM_OK && fvm_construct(c,"Object",0,0,&child)==FVM_OK);install(c,scope,"child",child);fvm_value_undefined(&value);assert(fvm_object_set(c,child,"member",&value)==FVM_OK);
 fvm_value_string(&name,"child.member");assert(fvm_exec_variable_path(c,scope,0,0,&name,&out,&handled,&present)==FVM_OK && handled && present && out.type==FVM_VALUE_UNDEFINED);
 failure=1;fvm_value_number(&out,99);handled=21;present=22;assert(fvm_exec_variable_path(c,scope,0,0,&name,&out,&handled,&present)==FVM_EXCEPTION && out.number==99 && handled==21 && present==22 && fvm_exception_clear(c)==FVM_OK);failure=0;
 detach=1;fvm_value_number(&value,17);assert(fvm_object_set(c,child,"member",&value)==FVM_OK);assert(fvm_exec_variable_path(c,scope,0,0,&name,&name,&handled,&present)==FVM_OK && handled && present && name.number==17);
 fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
