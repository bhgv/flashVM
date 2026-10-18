#include "flashvm/flashvm.h"
#include "flashvm/value.h"
#include "flashvm/gc.h"
#include <assert.h>
int main(void) {
    FVM_Config cfg; FVM_Runtime *r=0; FVM_Context *c=0;
    FVM_Object *o=0; unsigned long count=0, threshold=0, reclaimed=0; int mode=0;
    assert(fvm_config_init(&cfg)==FVM_OK);
    assert(fvm_runtime_create(&cfg,&r)==FVM_OK);
    assert(fvm_context_create(r,&c)==FVM_OK);
    assert(fvm_gc_mode(c,100)==FVM_OK);
    assert(fvm_gc_threshold(c,&threshold)==FVM_OK && threshold==100);
    assert(fvm_object_create(c,&o)==FVM_OK);
    assert(fvm_gc_step(c,&reclaimed)==FVM_OK && reclaimed==0);
    assert(fvm_gc_object_count(c,&count)==FVM_OK && count==2);
    assert(fvm_gc_mode(c,FVM_GC_SUSPENDED)==FVM_OK);
    assert(fvm_gc_step(c,&reclaimed)==FVM_OK && reclaimed==0);
    assert(fvm_gc_mode(c,FVM_GC_FULL)==FVM_OK);
    assert(fvm_gc_state(c,&mode)==FVM_OK); /* full preserves suspended */
    assert(mode==FVM_GC_SUSPENDED);
    assert(fvm_gc_object_count(c,&count)==FVM_OK && count==1);
    fvm_context_destroy(c); fvm_runtime_destroy(r);
    return 0;
}
