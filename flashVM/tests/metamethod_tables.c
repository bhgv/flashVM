#include "flashvm/value.h"
#include "flashvm/table.h"
#include <assert.h>
#include <string.h>
int main(void) {
    FVM_Config cfg;
    FVM_Runtime *runtime=0;
    FVM_Context *ctx=0;
    FVM_Object *target=0,*meta=0,*fallback=0,*writes=0,*old=0;
    FVM_Value v,got;
    fvm_config_init(&cfg);
    assert(fvm_runtime_create(&cfg,&runtime)==FVM_OK);
    assert(fvm_context_create(runtime,&ctx)==FVM_OK);
    assert(fvm_object_create(ctx,&target)==FVM_OK);
    assert(fvm_object_create(ctx,&meta)==FVM_OK);
    assert(fvm_object_create(ctx,&fallback)==FVM_OK);
    assert(fvm_object_create(ctx,&writes)==FVM_OK);
    fvm_value_number(&v,37);
    assert(fvm_object_set(ctx,fallback,"answer",&v)==FVM_OK);
    v.type=FVM_VALUE_OBJECT;v.object=fallback;
    assert(fvm_object_set(ctx,meta,"__index",&v)==FVM_OK);
    v.object=writes;
    assert(fvm_object_set(ctx,meta,"__newindex",&v)==FVM_OK);
    assert(fvm_table_get_metatable(ctx,target,&old)==FVM_OK && old==0);
    assert(fvm_table_set_metatable(ctx,target,meta)==FVM_OK);
    assert(fvm_object_get(ctx,target,"answer",&got)==FVM_OK);
    assert(got.type==FVM_VALUE_NUMBER && got.number==37);
    fvm_value_number(&v,99);
    assert(fvm_object_set(ctx,target,"redirected",&v)==FVM_OK);
    assert(fvm_object_get(ctx,writes,"redirected",&got)==FVM_OK);
    assert(got.type==FVM_VALUE_NUMBER && got.number==99);
    assert(fvm_table_set_metatable(ctx,target,0)==FVM_OK);
    assert(fvm_object_get(ctx,target,"answer",&got)==FVM_OK);
    assert(got.type==FVM_VALUE_UNDEFINED);
    fvm_context_destroy(ctx);fvm_runtime_destroy(runtime);
    return 0;
}
