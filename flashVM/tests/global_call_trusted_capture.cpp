/* Direct counted global calls honor trusted collection while rooting a self-replaced capture. */
#include "../src/core/runtime.hpp"
#include "../src/core/native_call_guard.hpp"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include <cassert>
static const char name[]={'r','u','n',0,'X'};
static unsigned calls=0,mode=0;
static bool enclosing=false;
static FVM_Result replacement(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{fvm_value_number(out,99);return FVM_OK;}
static FVM_Result original(FVM_Context *c,FVM_Object *,unsigned count,const FVM_Value *args,FVM_Value *out,void *raw)
{
    ++calls;assert(count==1 && args[0].type==FVM_VALUE_OBJECT);
    FVM_Object *capture=(FVM_Object *)raw;
    assert(fvm_register_function_rooted_bytes(c,name,sizeof(name),replacement,0,0) == FVM_OK);
    assert(c->gc_native_depth==(enclosing?1U:0U));
    assert(fvm_gc_collect(c,0) == (enclosing?FVM_INVALID_ARGUMENT:FVM_OK));
    FVM_Value marker;assert(fvm_object_get(c,capture,"marker",&marker) == FVM_OK && marker.number==55);
    assert(fvm_object_get(c,args[0].object,"marker",&marker) == FVM_OK && marker.number==55);
    *out=marker;
    if(mode==2) return fvm_exception_raise(c,&marker);
    return mode==1?FVM_ERROR:FVM_OK;
}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned outer=0;outer<2;++outer)for(mode=0;mode<3;++mode) {
        FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);
        FVM_Object *capture=0,*argument=0;assert(fvm_object_create(c,&capture)==FVM_OK && fvm_object_create(c,&argument)==FVM_OK);
        FVM_Value marker;fvm_value_number(&marker,55);assert(fvm_object_set(c,capture,"marker",&marker)==FVM_OK && fvm_object_set(c,argument,"marker",&marker)==FVM_OK);
        assert(fvm_register_function_rooted_bytes(c,name,sizeof(name),original,capture,capture)==FVM_OK);
        c->functions[fvm_detail::string(name,sizeof(name))].collector_entry=true;
        FVM_Value arg,out;fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=argument;
        const FVM_Result expected=mode==2?FVM_EXCEPTION:mode==1?FVM_ERROR:FVM_OK;
        calls=0;enclosing=outer!=0;
        if(outer) {FVM_NativeCallGuard barrier(c);assert(fvm_call_bytes(c,name,sizeof(name),1,&arg,&out)==expected);assert(c->gc_native_depth==1);}
        else assert(fvm_call_bytes(c,name,sizeof(name),1,&arg,&out)==expected);
        assert(calls==1 && !c->gc_frames && !c->gc_native_depth);
        if(mode==2)assert(fvm_exception_clear(c)==FVM_OK);
        fvm_value_undefined(&arg);fvm_value_undefined(&out);assert(fvm_gc_collect(c,0)==FVM_OK);
        for(size_t i=0;i<c->objects.size();++i)assert(c->objects[i]!=capture && c->objects[i]!=argument);
        assert(fvm_call_bytes(c,name,sizeof(name),0,0,&out)==FVM_OK && out.number==99);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
