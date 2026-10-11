#include "flashvm/flashvm.h"
#include "flashvm/value.h"
#include "flashvm/table.h"
#include "flashvm/metamethod.h"
#include <assert.h>
#include <string.h>
static int gc_count=0;
static FVM_Result on_gc(FVM_Context *c,FVM_Object *self,unsigned int n,const FVM_Value *a,FVM_Value *out,void *u) {
    (void)c;(void)u;assert(n==3);assert(a[0].object==self);assert(a[2].object==self);++gc_count;fvm_value_undefined(out);return FVM_OK;
}
static FVM_Result on_index(FVM_Context *c,FVM_Object *self,unsigned int n,const FVM_Value *a,FVM_Value *out,void *u) {
    (void)c;(void)u;assert(n==3);assert(a[0].object==self);assert(strcmp(a[2].string,"missing")==0);fvm_value_number(out,77);return FVM_OK;
}
static FVM_Result on_iter(FVM_Context *c,FVM_Object *self,unsigned int n,const FVM_Value *a,FVM_Value *out,void *u) {
    (void)c;(void)u;assert(n==3);assert(a[0].object==self);if(a[2].number==0)fvm_value_number(out,12);else fvm_value_undefined(out);return FVM_OK;
}
int main(void) {
    FVM_Config config;FVM_Runtime *r=0;FVM_Context *c=0;FVM_Object *o=0,*m=0;FVM_Value v,k;
    assert(fvm_config_init(&config)==FVM_OK);
    assert(fvm_runtime_create(&config,&r)==FVM_OK);
    assert(fvm_context_create(r,&c)==FVM_OK);
    assert(fvm_object_create(c,&o)==FVM_OK);
    assert(fvm_object_create(c,&m)==FVM_OK);
    assert(fvm_meta_register(c,m,"__gc",on_gc,0)==FVM_OK);
    assert(fvm_meta_register(c,m,"__index",on_index,0)==FVM_OK);
    assert(fvm_meta_register(c,m,"__iterate",on_iter,0)==FVM_OK);
    assert(fvm_table_set_metatable(c,o,m)==FVM_OK);
    assert(fvm_object_get(c,o,"missing",&v)==FVM_OK && v.number==77);
    assert(fvm_meta_iterate(c,o,0,&k,&v)==FVM_OK && v.number==12);
    assert(fvm_meta_iterate(c,o,1,&k,&v)==FVM_OK && v.type==FVM_VALUE_UNDEFINED);
    assert(fvm_meta_finalize(c,o)==FVM_OK && gc_count==1);
    assert(fvm_meta_finalize(c,o)==FVM_OK && gc_count==1);
    fvm_context_destroy(c);fvm_runtime_destroy(r);assert(gc_count==1);return 0;
}
