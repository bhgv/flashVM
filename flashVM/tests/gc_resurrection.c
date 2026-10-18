#include "flashvm/gc.h"
#include "flashvm/table.h"
#include "flashvm/metamethod.h"
#include <assert.h>
static int called=0;
static FVM_Result resurrect(FVM_Context *vm,FVM_Object *self,unsigned int argc,const FVM_Value *args,FVM_Value *result,void *ud) {
    FVM_Value v; (void)self;(void)argc;(void)result;(void)ud;
    ++called;
    fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=args[0].object;
    return fvm_set_global(vm,"resurrected",&v);
}
int main(void) {
    FVM_Config cfg;FVM_Runtime *r=0;FVM_Context *vm=0;
    FVM_Object *o=0,*meta=0;unsigned long freed=0;FVM_Value v;
    assert(fvm_config_init(&cfg)==FVM_OK);
    assert(fvm_runtime_create(&cfg,&r)==FVM_OK);
    assert(fvm_context_create(r,&vm)==FVM_OK);
    assert(fvm_object_create(vm,&o)==FVM_OK);
    assert(fvm_object_create(vm,&meta)==FVM_OK);
    assert(fvm_meta_register(vm,meta,"__gc",resurrect,0)==FVM_OK);
    assert(fvm_table_set_metatable(vm,o,meta)==FVM_OK);
    assert(fvm_gc_collect(vm,&freed)==FVM_OK);
    assert(called==1);
    assert(fvm_get_global(vm,"resurrected",&v)==FVM_OK && v.object==o);
    assert(fvm_gc_collect(vm,&freed)==FVM_OK && called==1);
    fvm_context_destroy(vm);fvm_runtime_destroy(r);return 0;
}
