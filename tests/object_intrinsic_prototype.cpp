/* Intrinsic prototype edges differ from own host accessors and user descriptors. */
#include "flashvm/standard.h"
#include "flashvm/property.h"
#include "flashvm/gc.h"
#include "../src/core/runtime.hpp"
#include <cassert>
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=4;version<=9;++version) {
        FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);
        if(version<=8) { unsigned char swf[]={'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK); }
        assert(fvm_standard_install(c)==FVM_OK);
        FVM_Value klass,proto,value;assert(fvm_get_global(c,"Object",&klass)==FVM_OK && fvm_object_get(c,klass.object,"prototype",&proto)==FVM_OK);
        int own=0;assert(fvm_object_has_own(c,proto.object,"__proto__",&own)==FVM_OK && own==(version==9));
        FVM_Object *o=0;assert(fvm_construct(c,"Object",0,0,&o)==FVM_OK && fvm_gc_pin(c,o)==FVM_OK);
        assert(fvm_object_has_own(c,o,"__proto__",&own)==FVM_OK && own==(version<=8));
        assert(fvm_object_get(c,o,"__PROTO__",&value)==FVM_OK);
        if(version<=6)assert(value.type==FVM_VALUE_OBJECT && value.object==proto.object);else assert(value.type==FVM_VALUE_UNDEFINED);
        /* Explicit case-variant properties are not replaced by the intrinsic fallback. */
        fvm_value_number(&value,17);assert(fvm_object_set(c,o,"__PROTO__",&value)==FVM_OK);
        assert(fvm_object_get(c,o,"__PROTO__",&value)==FVM_OK && value.type==FVM_VALUE_NUMBER && value.number==17);
        assert(fvm_object_has_own(c,o,"__PROTO__",&own)==FVM_OK && own);
        assert(fvm_object_set_property_flags(c,o,"__proto__",0,7)==FVM_OK);
        int deleted=0;assert(fvm_object_delete(c,o,"__proto__",&deleted)==FVM_OK && deleted);
        assert((o->prototype==0)==(version<=8));
        assert(fvm_object_get(c,o,"__proto__",&value)==FVM_OK);
        if(version<=8)assert(value.type==FVM_VALUE_UNDEFINED);else assert(value.type==FVM_VALUE_OBJECT);
        assert(fvm_gc_collect(c,0)==FVM_OK && fvm_object_has_own(c,o,"__proto__",&own)==FVM_OK && !own);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
