#include "flashvm/gc.h"
#include <assert.h>
int main(void) {
 FVM_Config cfg;FVM_Runtime *r=0;FVM_Context *vm=0;
 FVM_Object *root=0,*child=0;FVM_Value v;unsigned long freed=0,count=0;
 assert(fvm_config_init(&cfg)==FVM_OK);
 assert(fvm_runtime_create(&cfg,&r)==FVM_OK);
 assert(fvm_context_create(r,&vm)==FVM_OK);
 assert(fvm_object_create(vm,&root)==FVM_OK);
 assert(fvm_object_create(vm,&child)==FVM_OK);
 fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=child;
 assert(fvm_object_set(vm,root,"child",&v)==FVM_OK);
 assert(fvm_gc_pin(vm,root)==FVM_OK);
 assert(fvm_gc_pin(vm,root)==FVM_OK);
 assert(fvm_gc_collect(vm,&freed)==FVM_OK && freed==0);
 assert(fvm_gc_unpin(vm,root)==FVM_OK);
 assert(fvm_gc_collect(vm,&freed)==FVM_OK && freed==0);
 assert(fvm_gc_unpin(vm,root)==FVM_OK);
 assert(fvm_gc_collect(vm,&freed)==FVM_OK && freed==2);
 assert(fvm_gc_object_count(vm,&count)==FVM_OK && count==1);
 fvm_context_destroy(vm);fvm_runtime_destroy(r);return 0;
}
