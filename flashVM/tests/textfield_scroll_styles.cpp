/* Wrap, hard breaks, leading, underline, insets and automatic dimensions share one viewport model. */
#include "textfield_scroll_fixture.hpp"
int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned underline=0;underline<2;++underline)for(unsigned wrap=0;wrap<2;++wrap)
    {
        FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);FVM_Object *field=scroll_field(c),*format=0;FVM_Value v,arg,out;
        assert(fvm_construct(c,"TextFormat",0,0,&format)==FVM_OK);fvm_value_number(&v,40);assert(fvm_object_set(c,format,"size",&v)==FVM_OK);fvm_value_number(&v,6);assert(fvm_object_set(c,format,"leading",&v)==FVM_OK);fvm_value_boolean(&v,underline);assert(fvm_object_set(c,format,"underline",&v)==FVM_OK);
        fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=format;assert(fvm_object_call(c,field,"setTextFormat",1,&arg,&out)==FVM_OK && fvm_object_call(c,field,"setNewTextFormat",1,&arg,&out)==FVM_OK);
        scroll_number(c,field,"_width",52);scroll_number(c,field,"_height",48);fvm_value_boolean(&v,wrap);assert(fvm_object_set(c,field,"wordWrap",&v)==FVM_OK);
        assert(fvm_value_string(&v,wrap?"AV AV AV":"AV\r\nAV\nAV")==FVM_OK && fvm_object_set(c,field,"text",&v)==FVM_OK);
        FVM_TextFieldScrollInfo info=scroll_info(c,field);assert(info.line_count==(wrap?5U:3U) && info.line_height==50 && info.maxscroll==(wrap?5U:3U) && info.bottom_scroll==1);
        FVM_PathBounds before=scroll_bounds(c,field);scroll_number(c,field,"scroll",3);FVM_PathBounds after=scroll_bounds(c,field);assert(after.y_min==before.y_min-100 && after.y_max==before.y_max-100 && after.x_min==before.x_min);
        double intrinsic=counted_metric(c,field,"textHeight");assert(intrinsic==(wrap?244:144));
        assert(fvm_value_string(&v,"left")==FVM_OK && fvm_object_set(c,field,"autoSize",&v)==FVM_OK);info=scroll_info(c,field);assert(info.scroll==1 && info.maxscroll==1 && counted_metric(c,field,"_height")== (wrap?248:148));
        fvm_context_destroy(c);
    }
    FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);FVM_Object *field=scroll_field(c);FVM_Value v;
    const char counted[]={'A',0,'V','\n','A','\n'};assert(fvm_value_string_bytes(&v,counted,sizeof(counted))==FVM_OK && fvm_object_set(c,field,"text",&v)==FVM_OK);assert(scroll_info(c,field).line_count==3);
    fvm_value_boolean(&v,0);assert(fvm_object_set(c,field,"multiline",&v)==FVM_OK);
    FVM_TextFieldScrollInfo invalid;std::memset(&invalid,77,sizeof(invalid));invalid.struct_size=sizeof(invalid);FVM_TextFieldScrollInfo old=invalid;
    assert(fvm_textfield_scroll_info(c,field,&invalid)==FVM_UNSUPPORTED && !std::memcmp(&old,&invalid,sizeof(old)));
    assert(fvm_value_string_bytes(&v,counted,3)==FVM_OK && fvm_object_set(c,field,"text",&v)==FVM_OK && scroll_info(c,field).line_count==1);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);
#endif
    return 0;
}
