#include "flashvm/value.h"
#include <assert.h>
#include <string.h>
static FVM_Result add(FVM_Context *c,FVM_Object *self,unsigned int n,const FVM_Value *a,FVM_Value *out,void *user) {
    (void)c; (void)self; (void)user;
    if (n!=2 || a[0].type!=FVM_VALUE_NUMBER || a[1].type!=FVM_VALUE_NUMBER) return FVM_INVALID_ARGUMENT;
    fvm_value_number(out,a[0].number+a[1].number);
    return FVM_OK;
}
int main(void) {
    FVM_Config cfg;
    FVM_Runtime *rt=0;
    FVM_Context *ctx=0;
    FVM_Object *obj=0;
    FVM_Value v,got,args[2];
    char buffer[32]="hello";
    assert(fvm_config_init(&cfg)==FVM_OK);
    assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&ctx)==FVM_OK);
    assert(fvm_object_create(ctx,&obj)==FVM_OK);
    assert(fvm_value_string(&v,buffer)==FVM_OK);
    assert(fvm_object_set(ctx,obj,"message",&v)==FVM_OK);
    strcpy(buffer,"changed");
    assert(fvm_object_get(ctx,obj,"message",&got)==FVM_OK);
    assert(strcmp(got.string,"hello")==0);
    fvm_value_undefined(&v); v.type=FVM_VALUE_OBJECT; v.object=obj;
    assert(fvm_set_global(ctx,"object",&v)==FVM_OK);
    assert(fvm_get_global(ctx,"object",&got)==FVM_OK && got.object==obj);
    assert(fvm_register_function(ctx,"add",add,0)==FVM_OK);
    fvm_value_number(&args[0],12); fvm_value_number(&args[1],30);
    assert(fvm_call(ctx,"add",2,args,&got)==FVM_OK);
    assert(got.type==FVM_VALUE_NUMBER && got.number==42);
    fvm_context_destroy(ctx);
    fvm_runtime_destroy(rt);
    return 0;
}
