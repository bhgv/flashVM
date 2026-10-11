#include "flashvm/flashvm.h"
#include "flashvm/value.h"
#include "flashvm/gc.h"
#include <assert.h>
int main(void) {
    FVM_Config cfg; FVM_Runtime *r=0; FVM_Context *c=0;
    FVM_Object *o=0; FVM_GCStats stats;
    assert(fvm_config_init(&cfg)==FVM_OK);
    assert(fvm_runtime_create(&cfg,&r)==FVM_OK);
    assert(fvm_context_create(r,&c)==FVM_OK);
    stats.struct_size=sizeof(stats);
    assert(fvm_gc_stats(c,&stats)==FVM_OK);
    assert(stats.live_objects==1 && stats.pinned_objects==0);
    assert(fvm_object_create(c,&o)==FVM_OK);
    assert(fvm_gc_pin(c,o)==FVM_OK);
    assert(fvm_gc_stats(c,&stats)==FVM_OK);
    assert(stats.live_objects==2 && stats.pinned_objects==1);
    assert(stats.pending_allocations>=1 && stats.mode==0);
    assert(fvm_gc_unpin(c,o)==FVM_OK);
    assert(fvm_gc_collect(c,0)==FVM_OK);
    assert(fvm_gc_stats(c,&stats)==FVM_OK);
    assert(stats.live_objects==1 && stats.pending_allocations==0);
    stats.struct_size=0;
    assert(fvm_gc_stats(c,&stats)==FVM_INVALID_ARGUMENT);
    fvm_context_destroy(c); fvm_runtime_destroy(r);
    return 0;
}
