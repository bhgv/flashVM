#include "flashvm/gc.h"
#include "flashvm/metamethod.h"
#include "flashvm/table.h"
#include <assert.h>
static int finalized=0;
static FVM_Result on_gc(FVM_Context *c,FVM_Object *self,unsigned int argc,const FVM_Value *args,FVM_Value *result,void *user) {
    (void)c;(void)self;(void)result;(void)user;
    assert(argc==3 && args[0].type==FVM_VALUE_OBJECT);
    ++finalized;return FVM_OK;
}
int main(void) {
    FVM_Config cfg;FVM_Runtime *r=0;FVM_Context *c=0;
    FVM_Object *a=0,*b=0,*meta=0;FVM_Value v;unsigned long n=0;
    int mode=0;
    assert(fvm_config_init(&cfg)==FVM_OK);
    assert(fvm_runtime_create(&cfg,&r)==FVM_OK);
    assert(fvm_context_create(r,&c)==FVM_OK);
    assert(fvm_gc_mode(c,FVM_GC_SUSPENDED)==FVM_OK);
    assert(fvm_gc_state(c,&mode)==FVM_OK && mode==FVM_GC_SUSPENDED);
    assert(fvm_object_create(c,&a)==FVM_OK);
    assert(fvm_object_create(c,&b)==FVM_OK);
    assert(fvm_object_create(c,&meta)==FVM_OK);
    assert(fvm_meta_register(c,meta,"__gc",on_gc,0)==FVM_OK);
    assert(fvm_table_set_metatable(c,a,meta)==FVM_OK);
    assert(fvm_table_set_metatable(c,b,meta)==FVM_OK);
    fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=b;
    assert(fvm_object_set(c,a,"next",&v)==FVM_OK);
    v.object=a;assert(fvm_object_set(c,b,"next",&v)==FVM_OK);
    assert(fvm_gc_pin(c,meta)==FVM_OK);
    assert(fvm_gc_mode(c,FVM_GC_FULL)==FVM_OK);
    assert(finalized==2);
    assert(fvm_gc_object_count(c,&n)==FVM_OK && n==2);
    assert(fvm_gc_state(c,&mode)==FVM_OK && mode==FVM_GC_SUSPENDED);
    assert(fvm_gc_unpin(c,meta)==FVM_OK);
    assert(fvm_gc_collect(c,&n)==FVM_OK && n==1);
    assert(fvm_gc_mode(c,50)==FVM_OK);
    assert(fvm_gc_state(c,&mode)==FVM_OK && mode==50);
    fvm_context_destroy(c);fvm_runtime_destroy(r);
    return 0;
}
