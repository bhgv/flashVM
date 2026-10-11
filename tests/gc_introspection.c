#include "flashvm/gc.h"
#include "flashvm/object.h"
#include <assert.h>
int main(void) {
 FVM_Config cfg; FVM_Runtime *r=0;FVM_Context *c=0;FVM_Object *o=0;
 unsigned long x=0;
 assert(fvm_config_init(&cfg)==FVM_OK);
 assert(fvm_runtime_create(&cfg,&r)==FVM_OK);
 assert(fvm_context_create(r,&c)==FVM_OK);
 assert(fvm_gc_threshold(c,&x)==FVM_OK && x==1000UL);
 assert(fvm_gc_mode(c,100)==FVM_OK);
 assert(fvm_gc_threshold(c,&x)==FVM_OK && x==100UL);
 assert(fvm_gc_mode(c,-100)==FVM_OK);
 assert(fvm_gc_threshold(c,&x)==FVM_OK && x==1900UL);
 assert(fvm_gc_mode(c,FVM_GC_SUSPENDED)==FVM_OK);
 assert(fvm_gc_threshold(c,&x)==FVM_OK && x>=1900UL);
 assert(fvm_object_create(c,&o)==FVM_OK);
 assert(fvm_gc_pending_allocations(c,&x)==FVM_OK && x>=1UL);
 assert(fvm_gc_pin(c,o)==FVM_OK);
 assert(fvm_gc_pin_count(c,o,&x)==FVM_OK && x==1UL);
 assert(fvm_gc_unpin(c,o)==FVM_OK);
 assert(fvm_gc_pin_count(c,o,&x)==FVM_OK && x==0UL);
 assert(fvm_gc_threshold(0,&x)==FVM_INVALID_ARGUMENT);
 assert(fvm_gc_pending_allocations(c,0)==FVM_INVALID_ARGUMENT);
 fvm_context_destroy(c);fvm_runtime_destroy(r);return 0;
}
