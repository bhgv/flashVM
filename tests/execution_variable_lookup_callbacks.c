/* Plain lookup retains inputs through getter GC and preserves metadata on exceptions. */
#define main delete_path_fixture_main
#include "execution_delete_path.c"
#undef main
int main(void)
{
 FVM_Config cfg;FVM_Runtime *rt;FVM_Context *c;FVM_Object *scope,*leaf,*fallback,*owner;FVM_Value name,out;int found;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);
 assert(fvm_construct(c,"Object",0,0,&scope)==FVM_OK && fvm_gc_pin(c,scope)==FVM_OK && fvm_construct(c,"Object",0,0,&leaf)==FVM_OK && fvm_object_create(c,&fallback)==FVM_OK);install(c,scope,"field",leaf);
 fvm_value_string(&name,"field");assert(fvm_exec_variable_lookup(c,scope,0,0,fallback,1,&name,&out,&owner,&found)==FVM_OK && found && out.object==leaf && owner==fallback);
 failure=1;fvm_value_number(&out,99);owner=scope;found=99;assert(fvm_exec_variable_lookup(c,scope,0,0,fallback,1,&name,&out,&owner,&found)==FVM_EXCEPTION && out.number==99 && owner==scope && found==99 && fvm_exception_clear(c)==FVM_OK);failure=0;
 detach=1;assert(fvm_exec_variable_lookup(c,scope,0,0,fallback,1,&name,&name,&owner,&found)==FVM_OK && found && name.object==leaf && owner==fallback && fvm_gc_pin(c,leaf)==FVM_OK);
 assert(fvm_object_define_accessor_rooted(c,scope,"native",native_getter,0,leaf,leaf)==FVM_OK);fvm_value_string(&name,"native");assert(fvm_exec_variable_lookup(c,scope,0,0,fallback,1,&name,&out,&owner,&found)==FVM_OK && found && out.object==leaf);
 fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
