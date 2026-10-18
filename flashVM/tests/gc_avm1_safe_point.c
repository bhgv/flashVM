#include "flashvm/avm1.h"
#include "flashvm/gc.h"
#include "flashvm/value.h"
#include <assert.h>

/* Automatic collection is triggered by the AVM1 instruction boundary, not
 * by object allocation: newly returned native pointers remain caller-owned
 * only until the next collection unless pinned or rooted. */
int main(void) {
    FVM_Config cfg;
    FVM_Runtime *runtime=0;
    FVM_Context *context=0;
    FVM_Object *object=0;
    unsigned long count=0, pending=0;
    unsigned int i;
    static const unsigned char end_action[]={0};
    assert(fvm_config_init(&cfg)==FVM_OK);
    assert(fvm_runtime_create(&cfg,&runtime)==FVM_OK);
    assert(fvm_context_create(runtime,&context)==FVM_OK);
    assert(fvm_gc_mode(context,100)==FVM_OK);
    for (i=0;i<105;++i)
        assert(fvm_object_create(context,&object)==FVM_OK);
    assert(fvm_gc_pending_allocations(context,&pending)==FVM_OK && pending>=100);
    assert(fvm_avm1_execute(context,end_action,sizeof(end_action))==FVM_OK);
    assert(fvm_gc_object_count(context,&count)==FVM_OK && count==1);
    assert(fvm_gc_pending_allocations(context,&pending)==FVM_OK && pending==0);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    return 0;
}
