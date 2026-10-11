#include "flashvm/gc.h"
#include "flashvm/table.h"
#include "flashvm/metamethod.h"
#include <assert.h>
static FVM_Object *meta;
static int calls;
static int shutdown;
static FVM_Result spawn(FVM_Context *c,FVM_Object *self,unsigned int argc,
    const FVM_Value *args,FVM_Value *out,void *ud) {
    FVM_Object *newborn=0;
    unsigned long n;
    FVM_Value v;
    (void)argc;(void)args;(void)out;(void)ud;
    ++calls;
    assert(fvm_gc_collect(c,&n)==FVM_INVALID_ARGUMENT);
    fvm_context_destroy(c); /* Must not destroy a collecting context. */
    if(shutdown) {
        assert(fvm_object_create(c,&newborn)==FVM_INVALID_ARGUMENT);
        return FVM_OK;
    }
    assert(fvm_object_create(c,&newborn)==FVM_OK);
    assert(fvm_table_set_metatable(c,newborn,meta)==FVM_OK);
    fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=self;
    assert(fvm_object_set(c,newborn,"parent",&v)==FVM_OK);
    return FVM_ERROR; /* Report error, still complete sweep. */
}
int main(void) {
    FVM_Config cfg;FVM_Runtime *rt;FVM_Context *c;FVM_Object *o;
    unsigned long n;
    assert(fvm_config_init(&cfg)==FVM_OK);
    assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK);
    assert(fvm_object_create(c,&meta)==FVM_OK);
    assert(fvm_meta_register(c,meta,"__gc",spawn,0)==FVM_OK);
    assert(fvm_object_create(c,&o)==FVM_OK);
    assert(fvm_table_set_metatable(c,o,meta)==FVM_OK);
    assert(fvm_gc_collect(c,&n)==FVM_ERROR && calls==1 && n==0);
    assert(fvm_gc_object_count(c,&n)==FVM_OK && n==4);
    assert(fvm_gc_collect(c,&n)==FVM_ERROR && calls==2 && n==0);
    shutdown=1;
    fvm_context_destroy(c);
    assert(calls==3);
    fvm_runtime_destroy(rt);
    return 0;
}
