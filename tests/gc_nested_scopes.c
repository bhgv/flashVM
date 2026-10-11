#include "flashvm/gc.h"
#include "flashvm/gc_roots.h"
#include "flashvm/flashvm.h"
#include <assert.h>

int main(void) {
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    FVM_Object *a = 0;
    FVM_Object *b = 0;
    FVM_GCRootScope *outer = 0;
    FVM_GCRootScope *inner = 0;
    FVM_Value value;
    unsigned long count = 0;
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &context) == FVM_OK);
    assert(fvm_gc_roots_begin(context, &outer) == FVM_OK);
    assert(fvm_object_create(context, &a) == FVM_OK);
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = a;
    assert(fvm_gc_roots_add(outer, &value) == FVM_OK);
    assert(fvm_gc_roots_begin(context, &inner) == FVM_OK);
    assert(fvm_object_create(context, &b) == FVM_OK);
    value.object = b;
    assert(fvm_gc_roots_add(inner, &value) == FVM_OK);
    assert(fvm_gc_roots_end(outer) == FVM_INVALID_ARGUMENT);
    assert(fvm_gc_collect(context, &count) == FVM_OK);
    assert(fvm_gc_object_count(context, &count) == FVM_OK && count == 3);
    assert(fvm_gc_roots_end(inner) == FVM_OK);
    assert(fvm_gc_collect(context, &count) == FVM_OK && count == 1);
    assert(fvm_gc_object_count(context, &count) == FVM_OK && count == 2);
    assert(fvm_gc_roots_end(outer) == FVM_OK);
    assert(fvm_gc_collect(context, &count) == FVM_OK && count == 1);
    assert(fvm_gc_object_count(context, &count) == FVM_OK && count == 1);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    return 0;
}
