#include "flashvm/gc.h"
#include "flashvm/gc_roots.h"
#include "flashvm/flashvm.h"
#include <assert.h>
int main(void) {
    FVM_Config cfg;
    FVM_Runtime *runtime=0;
    FVM_Context *context=0;
    FVM_Object *object=0;
    FVM_Value value;
    FVM_GCRootScope *scope=0;
    unsigned long n=0;
    assert(fvm_config_init(&cfg)==FVM_OK);
    assert(fvm_runtime_create(&cfg,&runtime)==FVM_OK);
    assert(fvm_context_create(runtime,&context)==FVM_OK);
    assert(fvm_gc_roots_begin(context,&scope)==FVM_OK);
    assert(fvm_object_create(context,&object)==FVM_OK);
    fvm_value_undefined(&value);
    value.type=FVM_VALUE_OBJECT;
    value.object=object;
    assert(fvm_gc_roots_add(scope,&value)==FVM_OK);
    assert(fvm_gc_collect(context,&n)==FVM_OK);
    assert(fvm_gc_object_count(context,&n)==FVM_OK && n==2);
    assert(fvm_gc_roots_end(scope)==FVM_OK);
    assert(fvm_gc_collect(context,&n)==FVM_OK && n==1);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    return 0;
}
