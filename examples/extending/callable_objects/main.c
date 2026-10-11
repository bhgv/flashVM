#include "flashvm/callable.h"
#include <assert.h>
static FVM_Result add(FVM_Context *ctx,FVM_Stack *s,void *ud) {
    FVM_Value a,b,out;
    (void)ctx; (void)ud;
    if(fvm_stack_size(s)!=2) return FVM_INVALID_ARGUMENT;
    if(fvm_stack_get(s,1,&a)!=FVM_OK || fvm_stack_get(s,2,&b)!=FVM_OK)
        return FVM_ERROR;
    if(a.type!=FVM_VALUE_NUMBER || b.type!=FVM_VALUE_NUMBER) return FVM_ERROR;
    fvm_value_number(&out,a.number+b.number);
    fvm_stack_clear(s);
    return fvm_stack_push(s,&out);
}
int main(void) {
    FVM_Config cfg; FVM_Runtime *rt=0;
    FVM_Context *ctx;
    FVM_Object *fn,*obj;
    FVM_Value args[2],prop,result;
    int yes=0;
    assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    ctx=0; assert(fvm_context_create(rt,&ctx)==FVM_OK);
    assert(fvm_callable_create(ctx,add,0,&fn)==FVM_OK);
    assert(fvm_callable_is(ctx,fn,&yes)==FVM_OK && yes);
    assert(fvm_object_create(ctx,&obj)==FVM_OK);
    fvm_value_undefined(&prop); prop.type=FVM_VALUE_OBJECT; prop.object=fn;
    assert(fvm_object_set(ctx,obj,"add",&prop)==FVM_OK);
    assert(fvm_object_get(ctx,obj,"add",&prop)==FVM_OK);
    fvm_value_number(&args[0],19); fvm_value_number(&args[1],23);
    assert(fvm_callable_invoke(ctx,prop.object,2,args,&result)==FVM_OK);
    assert(result.type==FVM_VALUE_NUMBER && result.number==42);
    assert(fvm_callable_invoke(ctx,obj,0,0,&result)==FVM_UNSUPPORTED);
    fvm_context_destroy(ctx); fvm_runtime_destroy(rt);
    return 0;
}
