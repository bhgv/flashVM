/* Callback/getter failures stop delivery after commit; malformed registries fail before commit. */
#include "textfield_scroll_events_fixture.hpp"
#include "flashvm/exception.h"
#include "flashvm/property.h"
#ifdef FVM_HAS_FONTS
struct ScrollFailure {FVM_Object *field;unsigned calls;int kind;};
static FVM_Result fail_scroll(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *raw)
{
    ScrollFailure *failure=(ScrollFailure*)raw;++failure->calls;FVM_Value marker;fvm_value_number(&marker,77);assert(fvm_object_set(c,failure->field,"notificationMarker",&marker)==FVM_OK);
    if(failure->kind==1){fvm_value_number(out,91);return fvm_exception_raise(c,out);}if(failure->kind==2)throw 1;
    fvm_value_undefined(out);return failure->kind==3?FVM_ERROR:FVM_OK;
}
#endif
int main(int argc,char **)
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    FVM_Object *field=scroll_field(c),*first=0,*last=0;assert(fvm_object_create(c,&first)==FVM_OK && fvm_object_create(c,&last)==FVM_OK);
    ScrollFailure own={field,0,1},failure={field,0,0},tail={field,0,0};
    assert(fvm_object_define_method_rooted(c,field,"onScroller",fail_scroll,&own,field)==FVM_OK && fvm_object_define_method_rooted(c,first,"onScroller",fail_scroll,&failure,field)==FVM_OK && fvm_object_define_method_rooted(c,last,"onScroller",fail_scroll,&tail,field)==FVM_OK && scroll_listen(c,field,first) && scroll_listen(c,field,last));
    FVM_Value value;fvm_value_number(&value,2);assert(fvm_object_set(c,field,"scroll",&value)==FVM_EXCEPTION && own.calls==1 && !failure.calls && !tail.calls);assert(fvm_exception_clear(c)==FVM_OK && scroll_info(c,field).scroll==2 && counted_metric(c,field,"notificationMarker")==77);
    own.kind=0;failure.kind=argc>1?3:2;fvm_value_number(&value,1);assert(fvm_object_set(c,field,"scroll",&value)==FVM_ERROR && own.calls==2 && failure.calls==1 && !tail.calls && scroll_info(c,field).scroll==1);
    int deleted=0;assert(fvm_object_delete(c,first,"onScroller",&deleted)==FVM_OK && deleted);failure.kind=1;
    assert(fvm_object_define_accessor_rooted(c,first,"onScroller",fail_scroll,0,&failure,field)==FVM_OK);
    fvm_value_number(&value,2);assert(fvm_object_set(c,field,"scroll",&value)==FVM_EXCEPTION && own.calls==3 && failure.calls==2 && !tail.calls);assert(fvm_exception_clear(c)==FVM_OK && scroll_info(c,field).scroll==2);
    FVM_Value registry=field->properties[FVM_TEXT_LISTENER_KEY];fvm_value_number(&field->properties[FVM_TEXT_LISTENER_KEY],3);fvm_value_number(&value,1);
    assert(fvm_object_set(c,field,"scroll",&value)==FVM_INVALID_ARGUMENT && scroll_info(c,field).scroll==2 && own.calls==3);field->properties[FVM_TEXT_LISTENER_KEY]=registry;
    assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);fvm_context_destroy(c);fvm_runtime_destroy(rt);
#else
    (void)argc;
#endif
    return 0;
}
