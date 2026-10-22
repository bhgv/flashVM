/* Default intervals derive from transferred font data, not from a sender cache or custom table. */
#include "textfield_tabs_fixture.hpp"
#include "../src/standard/text_state.hpp"
#include "../src/standard/text_constants.h"
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned explicit_future=0;explicit_future<2;++explicit_future){FVM_Context *c=0,*d=0;assert(fvm_context_create(rt,&c)==FVM_OK && fvm_context_create(rt,&d)==FVM_OK && fvm_standard_install(c)==FVM_OK && fvm_standard_install(d)==FVM_OK && fvm_standard_text_install(d)==FVM_OK);
  FVM_Object *field=counted_field(c),*format=tabs_format(c,0,0);inset_apply(c,field,format,"setNewTextFormat");inset_string(c,field,"text","A\tV\tA");inset_string(c,field,"autoSize","right");inset_number(c,field,"_x",13);
  double stops[]={40,80};format=tabs_format(c,stops,explicit_future?2:0);inset_number(c,format,"size",20);inset_apply(c,field,format,"setNewTextFormat");
  FVM_TextFieldState state;assert(fvm_textfield_snapshot(field,state)==FVM_OK && state.current.tab_stops.empty() && !(state.current.alignment&FVM_TEXT_STYLE_HAS_TABS) && state.future.tab_stops.size()==(explicit_future?2U:0U));
  FVM_Value root,out;fvm_value_undefined(&root);root.type=FVM_VALUE_OBJECT;root.object=field;unsigned char *wire=0;size_t size=0;assert(fvm_value_serialize(c,&root,&wire,&size)==FVM_OK && fvm_value_deserialize(d,wire,size,&out)==FVM_OK);fvm_serialized_free(wire);FVM_Object *copy=out.object;assert(fvm_gc_pin(d,copy)==FVM_OK && counted_metric(d,copy,"textWidth")==199 && counted_metric(d,copy,"_width")==203 && counted_metric(d,copy,"_x")==13);
  FVM_Channel *channel=0;assert(fvm_channel_create(&channel)==FVM_OK && fvm_channel_send_value(c,channel,&root,0)==FVM_OK && fvm_channel_receive_value(d,channel,&out,0)==FVM_OK && counted_metric(d,out.object,"textWidth")==199);fvm_channel_destroy(channel);fvm_context_destroy(c);
  assert(fvm_gc_collect(d,0)==FVM_OK);inset_string(d,copy,"text","A\tV\tA");double expected=explicit_future?92:99.5;assert(counted_metric(d,copy,"textWidth")==expected && counted_metric(d,copy,"_width")==expected+4 && counted_metric(d,copy,"_x")==13+199-expected);
  FVM_Path *path=0;assert(fvm_textfield_path(d,copy,&path)==FVM_OK);fvm_context_destroy(d);assert(fvm_path_count(path)>0);fvm_path_destroy(path);
 }
 fvm_runtime_destroy(rt);
#endif
 return 0;
}
