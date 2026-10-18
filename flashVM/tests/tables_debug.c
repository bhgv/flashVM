#include "flashvm/debug.h"
#include <assert.h>
#include <string.h>
int main(void) {
    FVM_Config config; FVM_Runtime *rt=0; FVM_Context *c=0;
    FVM_Object *a=0,*meta=0,*found=0; FVM_Value v,out;
    size_t n=0; unsigned long key=0; const char *name=0;
    assert(fvm_config_init(&config)==FVM_OK);
    assert(fvm_runtime_create(&config,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK);
    assert(fvm_object_create(c,&a)==FVM_OK);
    assert(fvm_object_create(c,&meta)==FVM_OK);
    fvm_value_number(&v,17);
    assert(fvm_table_set_index(c,meta,3,&v)==FVM_OK);
    assert(fvm_table_set_metatable(c,a,meta)==FVM_OK);
    assert(fvm_table_get_index(c,a,3,&out)==FVM_OK && out.number==17);
    assert(fvm_table_set_metatable(c,meta,a)==FVM_INVALID_ARGUMENT);
    fvm_value_number(&v,23);
    assert(fvm_table_set_index(c,a,3,&v)==FVM_OK);
    assert(fvm_table_get_index(c,a,3,&out)==FVM_OK && out.number==23);
    assert(fvm_table_indexed_count(c,a,&n)==FVM_OK && n==1);
    assert(fvm_table_indexed_at(c,a,0,&key,&out)==FVM_OK && key==3 && out.number==23);
    assert(fvm_object_set(c,a,"score",&v)==FVM_OK);
    assert(fvm_table_named_at(c,a,0,&name,&out)==FVM_OK && strcmp(name,"score")==0);
    assert(fvm_debug_object_count(c,&n)==FVM_OK && n>=3);
    assert(fvm_debug_object_at(c,n-1,&found)==FVM_OK && found==meta);
    assert(fvm_debug_native_function_count(c,&n)==FVM_OK && n==0);
    fvm_context_destroy(c); fvm_runtime_destroy(rt);
    return 0;
}
