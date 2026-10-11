#include "flashvm/userdata.h"
#include "flashvm/gc.h"
#include "flashvm/table.h"
#include "flashvm/metamethod.h"
#include <assert.h>
typedef struct Payload {FVM_Context *c;int released;} Payload;
static void release(void *ud) {
    Payload *p=(Payload *)ud;FVM_Object *o=0;FVM_GCStats stats;
    ++p->released;
    stats.struct_size=sizeof(stats);assert(fvm_gc_stats(p->c,&stats)==FVM_OK);
    assert(fvm_object_create(p->c,&o)==FVM_INVALID_ARGUMENT);
    assert(fvm_gc_collect(p->c,0)==FVM_INVALID_ARGUMENT);
    fvm_context_destroy(p->c);
}
static FVM_Result resurrect(FVM_Context *c,FVM_Object *o,unsigned int n,const FVM_Value *a,FVM_Value *out,void *ud) {
    FVM_Value v;(void)n;(void)a;(void)out;(void)ud;
    fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=o;
    return fvm_set_global(c,"saved",&v);
}
int main(void) {
    FVM_Config cfg;FVM_Runtime *rt;FVM_Context *c;FVM_Object *o,*m;
    FVM_Value v;Payload p;unsigned long n;void *data;
    assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK);p.c=c;p.released=0;
    assert(fvm_object_create(c,&o)==FVM_OK);assert(fvm_object_create(c,&m)==FVM_OK);
    assert(fvm_userdata_attach(c,o,&p,release)==FVM_OK);
    assert(fvm_userdata_attach(c,o,&p,release)==FVM_INVALID_ARGUMENT);
    assert(fvm_userdata_get(c,o,&data)==FVM_OK && data==&p);
    assert(fvm_meta_register(c,m,"__gc",resurrect,0)==FVM_OK);
    assert(fvm_table_set_metatable(c,o,m)==FVM_OK);
    assert(fvm_gc_collect(c,&n)==FVM_OK && n==0 && p.released==0);
    fvm_value_undefined(&v);assert(fvm_set_global(c,"saved",&v)==FVM_OK);
    assert(fvm_gc_collect(c,&n)==FVM_OK && n==2 && p.released==1);
    assert(fvm_object_create(c,&o)==FVM_OK);assert(fvm_userdata_attach(c,o,&p,release)==FVM_OK);
    fvm_context_destroy(c);assert(p.released==2);fvm_runtime_destroy(rt);return 0;
}
