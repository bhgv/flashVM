/* C89 embedding example: named and numeric table slots + inspection. */
#include "flashvm/debug.h"
#include <stdio.h>
int main(void) {
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *vm=0;
    FVM_Object *table=0; FVM_Value v; size_t count=0;
    if (fvm_config_init(&cfg)!=FVM_OK ||
        fvm_runtime_create(&cfg,&rt)!=FVM_OK ||
        fvm_context_create(rt,&vm)!=FVM_OK) return 1;
    if (fvm_object_create(vm,&table)!=FVM_OK) return 2;
    fvm_value_number(&v,42);
    if (fvm_object_set(vm,table,"answer",&v)!=FVM_OK ||
        fvm_table_set_index(vm,table,0,&v)!=FVM_OK) return 3;
    if (fvm_table_named_count(vm,table,&count)!=FVM_OK) return 4;
    printf("Named variables: %lu\n",(unsigned long)count);
    if (fvm_debug_object_count(vm,&count)!=FVM_OK) return 5;
    printf("Allocated objects: %lu\n",(unsigned long)count);
    fvm_context_destroy(vm); fvm_runtime_destroy(rt);
    return 0;
}
