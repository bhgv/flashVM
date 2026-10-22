/* Current/future custom tables remain independent across binary/channel transfer and owner teardown. */
#include "textfield_tabs_fixture.hpp"
#include "../src/standard/text_state.hpp"
#include "../src/standard/text_constants.h"
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0,*d=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_context_create(rt,&d)==FVM_OK && fvm_standard_install(c)==FVM_OK && fvm_standard_install(d)==FVM_OK && fvm_standard_text_install(d)==FVM_OK);FVM_Object *field=counted_field(c);register_styles(c);double stops[]={40,80};FVM_Object *format=tabs_format(c,stops,2);inset_apply(c,field,format,"setNewTextFormat");inset_string(c,field,"text","A\tV\tA");inset_string(c,field,"autoSize","left");stops[0]=60;stops[1]=120;format=tabs_format(c,stops,2);select_style(c,format,FVM_TEXT_FONT_BOLD);inset_number(c,format,"size",20);inset_number(c,format,"leftMargin",2);inset_number(c,format,"rightMargin",2);inset_number(c,format,"blockIndent",2);inset_number(c,format,"indent",-2);inset_apply(c,field,format,"setNewTextFormat");FVM_TextFieldState state;assert(fvm_textfield_snapshot(field,state)==FVM_OK && state.current.tab_stops[0]==40 && state.future.tab_stops[0]==60 && (state.current.alignment&FVM_TEXT_STYLE_HAS_TABS));
 FVM_Value root,out,v;fvm_value_undefined(&root);root.type=FVM_VALUE_OBJECT;root.object=field;unsigned char *wire=0;size_t size=0;assert(fvm_value_serialize(c,&root,&wire,&size)==FVM_OK && fvm_value_deserialize(d,wire,size,&out)==FVM_OK);fvm_serialized_free(wire);FVM_Object *copy=out.object;assert(fvm_gc_pin(d,copy)==FVM_OK && counted_metric(d,copy,"textWidth")==104 && counted_metric(d,copy,"_width")==108);
 FVM_Channel *channel=0;assert(fvm_channel_create(&channel)==FVM_OK && fvm_channel_send_value(c,channel,&root,0)==FVM_OK && fvm_channel_receive_value(d,channel,&out,0)==FVM_OK && counted_metric(d,out.object,"textWidth")==104);fvm_channel_destroy(channel);fvm_context_destroy(c);assert(fvm_gc_collect(d,0)==FVM_OK);inset_string(d,copy,"text","A\tV\tA");assert(counted_metric(d,copy,"textWidth")==134 && counted_metric(d,copy,"_width")==144 && counted_metric(d,copy,"_height")==26);
 assert(fvm_object_call(d,copy,"getTextFormat",0,0,&out)==FVM_OK && fvm_object_get(d,out.object,"tabStops",&v)==FVM_OK && fvm_table_get_index(d,v.object,0,&out)==FVM_OK && out.number==60);FVM_Path *p=0;assert(fvm_textfield_path(d,copy,&p)==FVM_OK);fvm_context_destroy(d);assert(fvm_path_count(p)>0);fvm_path_destroy(p);fvm_runtime_destroy(rt);
#endif
 return 0;
}
