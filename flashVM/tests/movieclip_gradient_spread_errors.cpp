/* Spread coercion failures keep published drawing state and retain explicit callback effects. */
#include "gradient_spread_fixture.hpp"
#include "flashvm/exception.h"
struct SpreadFailure {FVM_Object *clip;unsigned calls,kind;};
static FVM_Result spread_failure(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *raw)
{SpreadFailure *failure=(SpreadFailure*)raw;++failure->calls;set(c,failure->clip,"coercionMarker",77);assert(fvm_gc_collect(c,0)==FVM_INVALID_ARGUMENT);if(failure->kind==1){fvm_value_number(out,91);return fvm_exception_raise(c,out);}if(failure->kind==2)throw 1;return FVM_ERROR;}
int main(int argc,char **)
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);FVM_Object *clip=0,*coercion=0;assert(fvm_movieclip_create(c,"root",1,&clip)==FVM_OK && fvm_gc_pin(c,clip)==FVM_OK && fvm_object_create(c,&coercion)==FVM_OK);FVM_Value args[8],out;spread_arguments(c,args,1);assert(fvm_object_call(c,clip,"beginGradientFill",5,args,&out)==FVM_OK);FVM_Object *data=clip->resource_result.object,*old=data->properties["gradient"].object;SpreadFailure failure={clip,0,1};assert(fvm_object_define_method_rooted(c,coercion,"toString",spread_failure,&failure,clip)==FVM_OK);ref(args+5,coercion);
    assert(fvm_object_call(c,clip,"beginGradientFill",6,args,&out)==FVM_EXCEPTION && failure.calls==1 && data->properties["gradient"].object==old);assert(fvm_exception_clear(c)==FVM_OK);failure.kind=argc>1?3:2;assert(fvm_object_call(c,clip,"beginGradientFill",6,args,&out)==FVM_ERROR && failure.calls==2 && data->properties["gradient"].object==old);FVM_Value marker;assert(fvm_object_get(c,clip,"coercionMarker",&marker)==FVM_OK && marker.number==77);assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
