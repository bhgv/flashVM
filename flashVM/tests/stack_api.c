#include "flashvm/stack.h"
#include <assert.h>
static FVM_Result sum(FVM_Context *ctx,FVM_Stack *s,void *user) {
    FVM_Value a,b,result;
    (void)ctx;(void)user;
    if(fvm_stack_size(s)!=2 || fvm_stack_get(s,1,&a)!=FVM_OK || fvm_stack_get(s,-1,&b)!=FVM_OK) return FVM_ERROR;
    if(a.type!=FVM_VALUE_NUMBER || b.type!=FVM_VALUE_NUMBER) return FVM_ERROR;
    fvm_value_number(&result,a.number+b.number);
    if(fvm_stack_clear(s)!=FVM_OK) return FVM_ERROR;
    return fvm_stack_push(s,&result);
}
int main(void) {
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *ctx=0; FVM_Value args[2],out;
    assert(fvm_config_init(&cfg)==FVM_OK);
    assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&ctx)==FVM_OK);
    assert(fvm_stack_register(ctx,"sum",sum,0)==FVM_OK);
    fvm_value_number(&args[0],19); fvm_value_number(&args[1],23);
    assert(fvm_stack_call(ctx,"sum",2,args,&out)==FVM_OK);
    assert(out.type==FVM_VALUE_NUMBER && out.number==42);
    fvm_context_destroy(ctx);fvm_runtime_destroy(rt);return 0;
}
