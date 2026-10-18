#include "flashvm/gc.h"
#include "flashvm/table.h"
#include <assert.h>
int main(void) {
 FVM_Config cfg;FVM_Runtime *r=0;FVM_Context *vm=0;
 FVM_Object *a=0,*b=0,*m=0;unsigned long freed=0;
 assert(fvm_config_init(&cfg)==FVM_OK);
 assert(fvm_runtime_create(&cfg,&r)==FVM_OK);
 assert(fvm_context_create(r,&vm)==FVM_OK);
 assert(fvm_object_create(vm,&a)==FVM_OK);
 assert(fvm_object_create(vm,&b)==FVM_OK);
 assert(fvm_object_create(vm,&m)==FVM_OK);
 assert(fvm_object_set_prototype(vm,a,b)==FVM_OK);
 assert(fvm_table_set_metatable(vm,b,m)==FVM_OK);
 assert(fvm_table_set_metatable(vm,m,a)==FVM_OK);
 assert(fvm_gc_collect(vm,&freed)==FVM_OK && freed==3);
 fvm_context_destroy(vm);fvm_runtime_destroy(r);return 0;
}
