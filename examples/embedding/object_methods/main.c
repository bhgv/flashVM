/* C89 example: call a native method with an explicit ActionScript receiver. */
#include "flashvm/value.h"
#include <stdio.h>
static FVM_Result add(FVM_Context *ctx, FVM_Object *self,
                      unsigned int argc, const FVM_Value *args,
                      FVM_Value *result, void *userdata) {
    FVM_Value base;
    (void)userdata;
    if(argc!=1 || args[0].type!=FVM_VALUE_NUMBER) return FVM_INVALID_ARGUMENT;
    if(fvm_object_get(ctx,self,"base",&base)!=FVM_OK || base.type!=FVM_VALUE_NUMBER)
        return FVM_ERROR;
    fvm_value_number(result,base.number+args[0].number);
    return FVM_OK;
}
int main(void) {
    FVM_Config config; FVM_Runtime *runtime=0; FVM_Context *ctx=0;
    FVM_Object *obj=0; FVM_Value base,arg,result;
    if(fvm_config_init(&config)!=FVM_OK || fvm_runtime_create(&config,&runtime)!=FVM_OK) return 1;
    if(fvm_context_create(runtime,&ctx)!=FVM_OK) return 2;
    if(fvm_object_create(ctx,&obj)!=FVM_OK) return 3;
    fvm_value_number(&base,35);
    if(fvm_object_set(ctx,obj,"base",&base)!=FVM_OK) return 4;
    if(fvm_register_function(ctx,"add",add,0)!=FVM_OK) return 5;
    fvm_value_number(&arg,7);
    if(fvm_object_call(ctx,obj,"add",1,&arg,&result)!=FVM_OK) return 6;
    printf("Result: %.0f\n",result.number); /* Expected: 42 */
    fvm_context_destroy(ctx); fvm_runtime_destroy(runtime);
    return result.type==FVM_VALUE_NUMBER && result.number==42 ? 0 : 7;
}
