/* Conversion exceptions preserve setter ownership while explicit conversion side effects survive. */
#include "textfield_scroll_fixture.hpp"
#include "flashvm/exception.h"
#ifdef FVM_HAS_FONTS
struct ScrollConversion { FVM_Object *field;unsigned calls;bool edit; };
static FVM_Result scroll_throw(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *raw)
{
    ScrollConversion *conversion=(ScrollConversion*)raw;++conversion->calls;
    assert(c->gc_native_depth && fvm_gc_collect(c,0)==FVM_INVALID_ARGUMENT);
    if(conversion->edit)scroll_number(c,conversion->field,"scroll",1);
    fvm_value_number(out,91);return fvm_exception_raise(c,out);
}
#endif
int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    FVM_Object *field=scroll_field(c),*request=0;scroll_number(c,field,"scroll",2);scroll_number(c,field,"hscroll",7);
    assert(fvm_object_create(c,&request)==FVM_OK && fvm_gc_pin(c,request)==FVM_OK);
    ScrollConversion conversion;conversion.field=field;conversion.calls=0;conversion.edit=false;
    FVM_NativeEntry entry;entry.callback=scroll_throw;entry.userdata=&conversion;entry.root=field;request->methods["valueOf"]=entry;
    FVM_Value v;fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=request;
    for(unsigned side_effect=0;side_effect<2;++side_effect)
    {
        conversion.edit=side_effect!=0;
        assert(fvm_object_set(c,field,"hscroll",&v)==FVM_EXCEPTION && conversion.calls==side_effect+1);
        assert(fvm_exception_clear(c)==FVM_OK);
        FVM_TextFieldScrollInfo i=scroll_info(c,field);assert(i.scroll==(side_effect?1U:2U) && i.hscroll==7);
        assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth);
    }
    fvm_context_destroy(c);fvm_runtime_destroy(rt);
#endif
    return 0;
}
