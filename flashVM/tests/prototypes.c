#include "flashvm/value.h"
#include <assert.h>
int main(void) {
    FVM_Runtime *r=0;
    FVM_Context *c=0;
    FVM_Object *base=0, *child=0, *found=0;
    FVM_Value v, out;
    int own=0;
    { FVM_Config cfg; assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&r)==FVM_OK); }
    assert(fvm_context_create(r,&c)==FVM_OK);
    assert(fvm_object_create(c,&base)==FVM_OK);
    assert(fvm_object_create(c,&child)==FVM_OK);
    fvm_value_number(&v,12);
    assert(fvm_object_set(c,base,"answer",&v)==FVM_OK);
    assert(fvm_object_set_prototype(c,child,base)==FVM_OK);
    assert(fvm_object_get_prototype(c,child,&found)==FVM_OK && found==base);
    assert(fvm_object_get(c,child,"answer",&out)==FVM_OK && out.number==12);
    assert(fvm_object_has_own(c,child,"answer",&own)==FVM_OK && !own);
    fvm_value_number(&v,34);
    assert(fvm_object_set(c,child,"answer",&v)==FVM_OK);
    assert(fvm_object_get(c,child,"answer",&out)==FVM_OK && out.number==34);
    assert(fvm_object_has_own(c,child,"answer",&own)==FVM_OK && own);
    assert(fvm_object_set_prototype(c,base,child)==FVM_INVALID_ARGUMENT);
    assert(fvm_object_set_prototype(c,child,0)==FVM_OK);
    fvm_context_destroy(c);
    fvm_runtime_destroy(r);
    return 0;
}
