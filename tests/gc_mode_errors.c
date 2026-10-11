#include "flashvm/gc.h"
#include <assert.h>
int main(void) {FVM_Config cfg;FVM_Runtime *r=0;FVM_Context *c=0;int mode=0;unsigned long n=0;
 assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&r)==FVM_OK);assert(fvm_context_create(r,&c)==FVM_OK);
 assert(fvm_gc_mode(c,-101)==FVM_INVALID_ARGUMENT);assert(fvm_gc_mode(c,103)==FVM_INVALID_ARGUMENT);
 assert(fvm_gc_mode(c,101)==FVM_OK);assert(fvm_gc_mode(c,102)==FVM_OK);
 assert(fvm_gc_state(c,&mode)==FVM_OK && mode==101);
 assert(fvm_gc_step(c,&n)==FVM_OK && n==0UL);
 assert(fvm_gc_pin_count(c,0,&n)==FVM_INVALID_ARGUMENT);
 assert(fvm_gc_threshold(c,0)==FVM_INVALID_ARGUMENT);
 fvm_context_destroy(c);fvm_runtime_destroy(r);return 0;}
