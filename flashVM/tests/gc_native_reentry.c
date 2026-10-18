#include "flashvm/flashvm.h"
#include "flashvm/value.h"
#include "flashvm/gc.h"
#include <assert.h>

static FVM_Result native_check(FVM_Context *ctx, FVM_Object *self,
    unsigned int argc, const FVM_Value *args, FVM_Value *out, void *data) {
    unsigned long count=99;
    (void)self; (void)argc; (void)args; (void)data;
    assert(fvm_gc_collect(ctx,&count)==FVM_INVALID_ARGUMENT);
    assert(fvm_gc_step(ctx,&count)==FVM_OK);
    assert(count==0);
    fvm_value_number(out,42);
    return FVM_OK;
}
int main(void) {
    FVM_Config cfg; FVM_Runtime *r=0; FVM_Context *c=0; FVM_Value result;
    assert(fvm_config_init(&cfg)==FVM_OK);
    assert(fvm_runtime_create(&cfg,&r)==FVM_OK);
    assert(fvm_context_create(r,&c)==FVM_OK);
    assert(fvm_register_function(c,"native_check",native_check,0)==FVM_OK);
    assert(fvm_call(c,"native_check",0,0,&result)==FVM_OK);
    assert(result.type==FVM_VALUE_NUMBER && result.number==42);
    assert(fvm_gc_collect(c,0)==FVM_OK);
    fvm_context_destroy(c); fvm_runtime_destroy(r);
    return 0;
}
