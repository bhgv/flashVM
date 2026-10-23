/* Nondefault viewport ownership survives sender destruction, binary/channel transfer and GC. */
#include "textfield_scroll_fixture.hpp"
#include "../src/standard/text_state.hpp"
#include <limits>
int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0,*d=0,*e=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_context_create(rt,&d)==FVM_OK && fvm_context_create(rt,&e)==FVM_OK);
    FVM_Object *field=scroll_field(c);scroll_number(c,field,"scroll",2);scroll_number(c,field,"hscroll",7);scroll_number(c,field,"_x",-19);scroll_number(c,field,"_y",23);
    FVM_PathBounds expected=scroll_bounds(c,field);FVM_Value root,out;fvm_value_undefined(&root);root.type=FVM_VALUE_OBJECT;root.object=field;
    FVM_TextFieldState state;assert(fvm_textfield_snapshot(field,state)==FVM_OK && state.scroll==2 && state.hscroll==7);
    /* Emulate retired serialized native stubs, while captured host entries must not upgrade. */
    const char *names[]={"scroll","hscroll","maxscroll","maxhscroll","bottomScroll"};
    for(unsigned n=0;n<5;++n){FVM_AccessorEntry &a=field->accessors[names[n]];a.getter=a.setter=fvm_text_callback_from_id(205);a.userdata=0;a.root=0;FVM_AccessorEntry captured=a;captured.root=field;fvm_textfield_upgrade_accessor(names[n],captured);assert(captured.getter==a.getter && captured.setter==a.setter);}
    unsigned char *wire=0;size_t size=0;assert(fvm_value_serialize(c,&root,&wire,&size)==FVM_OK && fvm_standard_text_install(d)==FVM_OK && fvm_value_deserialize(d,wire,size,&out)==FVM_OK && fvm_gc_pin(d,out.object)==FVM_OK);
    FVM_Object *copy=out.object;
    FVM_Channel *channel=0;assert(fvm_channel_create(&channel)==FVM_OK && fvm_channel_send_value(c,channel,&root,0)==FVM_OK);
    fvm_context_destroy(c);
    assert(fvm_standard_text_install(e)==FVM_OK && fvm_channel_receive_value(e,channel,&out,0)==FVM_OK && fvm_gc_pin(e,out.object)==FVM_OK);fvm_channel_destroy(channel);
    FVM_Object *copies[]={copy,out.object};FVM_Context *contexts[]={d,e};
    for(unsigned j=0;j<2;++j){assert(fvm_gc_collect(contexts[j],0)==FVM_OK);FVM_TextFieldScrollInfo i=scroll_info(contexts[j],copies[j]);assert(i.scroll==2 && i.hscroll==7 && i.bottom_scroll==3 && counted_metric(contexts[j],copies[j],"_x")==-19);FVM_PathBounds b=scroll_bounds(contexts[j],copies[j]);assert(!std::memcmp(&b,&expected,sizeof(b)));scroll_number(contexts[j],copies[j],"scroll",1);assert(counted_metric(contexts[j],copies[j],"scroll")==1);}
    for(unsigned bad=0;bad<8;++bad)
    {
        FVM_TextFieldState invalid=state;FVM_Object *object=0;
        if(bad==0)invalid.scroll=0;else if(bad==1)invalid.scroll=1.5;else if(bad==2)invalid.scroll=3;else if(bad==3)invalid.hscroll=-1;else if(bad==4)invalid.hscroll=13;else if(bad==5)invalid.hscroll=0.5;else if(bad==6)invalid.scroll=std::numeric_limits<double>::quiet_NaN();else invalid.hscroll=2147483648.0;
        assert(fvm_object_create(d,&object)==FVM_OK && fvm_textfield_restore(d,object,invalid)==FVM_INVALID_ARGUMENT && !object->resource);
    }
    assert(fvm_value_deserialize(d,wire,size-1,&out)!=FVM_OK);fvm_serialized_free(wire);
    FVM_TextFieldState legacy;assert(legacy.scroll==1 && legacy.hscroll==0);
    fvm_context_destroy(d);fvm_context_destroy(e);fvm_runtime_destroy(rt);
#endif
    return 0;
}
