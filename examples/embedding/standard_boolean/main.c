#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    FVM_Object *boxed = 0;
    FVM_Value input, value, out;
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &context) == FVM_OK);
    assert(fvm_standard_install(context) == FVM_OK);
    fvm_value_boolean(&input, 0);
    assert(fvm_construct(context, "Boolean", 1, &input, &boxed) == FVM_OK);
    assert(fvm_gc_pin(context, boxed) == FVM_OK);
    assert(fvm_gc_collect(context, 0) == FVM_OK);
    assert(fvm_object_call(context, boxed, "valueOf", 0, 0, &out) == FVM_OK);
    assert(out.type == FVM_VALUE_BOOLEAN && !out.boolean);
    assert(fvm_object_call(context, boxed, "toString", 0, 0, &out) == FVM_OK);
    assert(strcmp(out.string, "false") == 0);
    printf("new Boolean(false).toString() = %s\n", out.string);
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = boxed;
    assert(fvm_call(context, "Boolean", 1, &value, &out) == FVM_OK && out.boolean);
    puts("Boolean(boxed false) = true (the wrapper is an object)");
    assert(fvm_call(context, "Number", 1, &value, &out) == FVM_OK && out.number == 0);
    assert(fvm_gc_unpin(context, boxed) == FVM_OK);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    return 0;
}
