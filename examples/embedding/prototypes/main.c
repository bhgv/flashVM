/* C89: prototype inheritance and property shadowing from a host application. */
#include "flashvm/value.h"
#include <stdio.h>
int main(void) {
    FVM_Runtime *runtime=0;
    FVM_Context *context=0;
    FVM_Object *base=0, *instance=0;
    FVM_Value value, result;
    { FVM_Config cfg; if (fvm_config_init(&cfg)!=FVM_OK || fvm_runtime_create(&cfg,&runtime)!=FVM_OK) return 1; }
    if (fvm_context_create(runtime,&context)!=FVM_OK) return 2;
    if (fvm_object_create(context,&base)!=FVM_OK) return 3;
    if (fvm_object_create(context,&instance)!=FVM_OK) return 4;
    fvm_value_number(&value,42);
    if (fvm_object_set(context,base,"inherited",&value)!=FVM_OK) return 5;
    if (fvm_object_set_prototype(context,instance,base)!=FVM_OK) return 6;
    if (fvm_object_get(context,instance,"inherited",&result)!=FVM_OK) return 7;
    printf("Inherited value: %.0f\n",result.number);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    return result.number==42 ? 0 : 8;
}
