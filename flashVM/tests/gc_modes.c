#include "flashvm/gc.h"
#include <assert.h>
int main(void) {
 FVM_Config cfg;FVM_Runtime *r=0;FVM_Context *vm=0;
 FVM_Object *o=0;unsigned long freed=99;int mode=999;
 assert(fvm_config_init(&cfg)==FVM_OK);
 assert(fvm_runtime_create(&cfg,&r)==FVM_OK);
 assert(fvm_context_create(r,&vm)==FVM_OK);
 assert(fvm_gc_mode(vm,-100)==FVM_OK);
 assert(fvm_gc_state(vm,&mode)==FVM_OK && mode==-100);
 assert(fvm_gc_mode(vm,100)==FVM_OK);
 assert(fvm_gc_state(vm,&mode)==FVM_OK && mode==100);
 assert(fvm_gc_mode(vm,101)==FVM_OK);
 assert(fvm_object_create(vm,&o)==FVM_OK);
 assert(fvm_gc_step(vm,&freed)==FVM_OK && freed==0);
 assert(fvm_gc_mode(vm,102)==FVM_OK);
 assert(fvm_gc_state(vm,&mode)==FVM_OK && mode==101);
 assert(fvm_gc_mode(vm,-101)==FVM_INVALID_ARGUMENT);
 assert(fvm_gc_mode(vm,103)==FVM_INVALID_ARGUMENT);
 assert(fvm_gc_mode(vm,0)==FVM_OK);
 fvm_context_destroy(vm);fvm_runtime_destroy(r);return 0;
}
