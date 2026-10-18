/* Prefix getters may collect/detach; final presence keeps undefined from falling through. */
#define main delete_path_fixture_main
#include "execution_delete_path.c"
#undef main
int main(void)
{
 FVM_Config cfg;FVM_Runtime *rt;FVM_Context *c;FVM_Object *inner,*outer,*bad,*good,*leaf,*scope;FVM_Value value,path,name,out;int present,removed;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);
 assert(fvm_construct(c,"Object",0,0,&inner)==FVM_OK && fvm_construct(c,"Object",0,0,&outer)==FVM_OK && fvm_construct(c,"Object",0,0,&bad)==FVM_OK && fvm_construct(c,"Object",0,0,&good)==FVM_OK && fvm_construct(c,"Object",0,0,&leaf)==FVM_OK);
 install(c,inner,"first",bad);install(c,outer,"first",good);
 fvm_value_undefined(&value);assert(fvm_object_set(c,bad,"second",&value)==FVM_OK);
 value.type=FVM_VALUE_OBJECT;value.object=leaf;assert(fvm_object_set(c,good,"second",&value)==FVM_OK);value.object=inner;assert(fvm_exec_with_scope(c,outer,&value,&scope)==FVM_OK && fvm_gc_pin(c,scope)==FVM_OK);
 fvm_value_string(&path,"first");fvm_value_string(&name,"second");calls=0;
 assert(fvm_exec_scope_member(c,scope,0,0,&path,&name,&out,&present)==FVM_OK && present && out.type==FVM_VALUE_UNDEFINED && calls==1);
 assert(fvm_object_delete(c,bad,"second",&removed)==FVM_OK && removed);calls=0;
 assert(fvm_exec_scope_member(c,scope,0,0,&path,&name,&out,&present)==FVM_OK && present && out.object==leaf && calls==2);
 failure=1;fvm_value_number(&out,99);present=99;assert(fvm_exec_scope_member(c,scope,0,0,&path,&name,&out,&present)==FVM_EXCEPTION && out.number==99 && present==99 && fvm_exception_clear(c)==FVM_OK);failure=0;
 detach=1;assert(fvm_exec_scope_member(c,scope,0,0,&path,&name,&name,&present)==FVM_OK && present && name.object==leaf);
 assert(fvm_gc_pin(c,leaf)==FVM_OK && fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
