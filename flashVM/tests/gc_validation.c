#include "flashvm/gc.h"
#include <assert.h>
int main(void) {
 FVM_Config cfg;FVM_Runtime *r=0;FVM_Context *a=0,*b=0;
 FVM_Object *obj=0;unsigned long n=0;
 assert(fvm_config_init(&cfg)==FVM_OK);
 assert(fvm_runtime_create(&cfg,&r)==FVM_OK);
 assert(fvm_context_create(r,&a)==FVM_OK);
 assert(fvm_context_create(r,&b)==FVM_OK);
 assert(fvm_object_create(a,&obj)==FVM_OK);
 assert(fvm_gc_pin(b,obj)==FVM_INVALID_ARGUMENT);
 assert(fvm_gc_unpin(a,obj)==FVM_INVALID_ARGUMENT);
 assert(fvm_gc_collect(0,&n)==FVM_INVALID_ARGUMENT);
 assert(fvm_gc_state(a,0)==FVM_INVALID_ARGUMENT);
 assert(fvm_gc_object_count(a,0)==FVM_INVALID_ARGUMENT);
 assert(fvm_gc_pin(a,obj)==FVM_OK);
 assert(fvm_gc_unpin(b,obj)==FVM_INVALID_ARGUMENT);
 assert(fvm_gc_unpin(a,obj)==FVM_OK);
 fvm_context_destroy(b);fvm_context_destroy(a);fvm_runtime_destroy(r);return 0;
}
