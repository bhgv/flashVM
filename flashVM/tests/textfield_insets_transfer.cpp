/* Paragraph insets preserve independent current/future styles across binary and channel transfer. */
#define FVM_FONT_FACES_FIXTURE_ONLY
#include "textfield_font_faces.cpp"
#include "../src/standard/text_state.hpp"
#include "../src/standard/text_constants.h"
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0,*d=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_context_create(rt,&d)==FVM_OK);
 FVM_Object *field=counted_field(c),*format=0;register_styles(c);FVM_Value arg,v,out,root;assert(fvm_construct(c,"TextFormat",0,0,&format)==FVM_OK);
 fvm_value_number(&v,40);assert(fvm_object_set(c,format,"size",&v)==FVM_OK);assert(fvm_value_string(&v,"justify")==FVM_OK && fvm_object_set(c,format,"align",&v)==FVM_OK);
 fvm_value_boolean(&v,1);assert(fvm_object_set(c,field,"multiline",&v)==FVM_OK && fvm_object_set(c,field,"wordWrap",&v)==FVM_OK);fvm_value_number(&v,124);assert(fvm_object_set(c,field,"_width",&v)==FVM_OK);
 fvm_value_number(&v,10);assert(fvm_object_set(c,format,"leftMargin",&v)==FVM_OK);fvm_value_number(&v,12);assert(fvm_object_set(c,format,"rightMargin",&v)==FVM_OK);fvm_value_number(&v,8);assert(fvm_object_set(c,format,"blockIndent",&v)==FVM_OK);
 fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=format;assert(fvm_object_call(c,field,"setNewTextFormat",1,&arg,&out)==FVM_OK && fvm_value_string(&v,"A V A V")==FVM_OK && fvm_object_set(c,field,"text",&v)==FVM_OK && fvm_value_string(&v,"left")==FVM_OK && fvm_object_set(c,field,"autoSize",&v)==FVM_OK);
 fvm_value_number(&v,2);assert(fvm_object_set(c,format,"leftMargin",&v)==FVM_OK && fvm_object_set(c,format,"rightMargin",&v)==FVM_OK && fvm_object_set(c,format,"blockIndent",&v)==FVM_OK);select_style(c,format,FVM_TEXT_FONT_BOLD);fvm_value_number(&v,20);assert(fvm_object_set(c,format,"size",&v)==FVM_OK && fvm_value_string(&v,"right")==FVM_OK && fvm_object_set(c,format,"align",&v)==FVM_OK && fvm_object_call(c,field,"setNewTextFormat",1,&arg,&out)==FVM_OK);
 FVM_TextFieldState state;assert(fvm_textfield_snapshot(field,state)==FVM_OK && (state.current.alignment&FVM_TEXT_STYLE_JUSTIFY) && state.current.left_margin==10 && state.current.right_margin==12 && state.current.block_indent==8 && state.future.left_margin==2 && (state.current.alignment&FVM_TEXT_STYLE_HAS_INSETS) && !(state.future.alignment&FVM_TEXT_STYLE_JUSTIFY));
 fvm_value_undefined(&root);root.type=FVM_VALUE_OBJECT;root.object=field;unsigned char *wire=0;size_t size=0;assert(fvm_standard_text_install(d)==FVM_OK && fvm_value_serialize(c,&root,&wire,&size)==FVM_OK && fvm_value_deserialize(d,wire,size,&out)==FVM_OK);fvm_serialized_free(wire);
 FVM_Object *copy=out.object;assert(fvm_gc_pin(d,copy)==FVM_OK && counted_metric(d,copy,"textWidth")==90 && counted_metric(d,copy,"_height")==94);
 assert(fvm_object_call(d,copy,"getTextFormat",0,0,&out)==FVM_OK && fvm_object_get(d,out.object,"align",&v)==FVM_OK && !std::strcmp(v.string,"justify"));
 FVM_Channel *ch=0;assert(fvm_channel_create(&ch)==FVM_OK && fvm_channel_send_value(c,ch,&root,0)==FVM_OK && fvm_channel_receive_value(d,ch,&out,0)==FVM_OK);fvm_channel_destroy(ch);assert(counted_metric(d,out.object,"textWidth")==90);
 fvm_context_destroy(c);assert(fvm_gc_collect(d,0)==FVM_OK && fvm_value_string(&v,"A V A V")==FVM_OK && fvm_object_set(d,copy,"text",&v)==FVM_OK && counted_metric(d,copy,"textWidth")==74 && counted_metric(d,copy,"_height")==26);
 assert(fvm_object_call(d,copy,"getTextFormat",0,0,&out)==FVM_OK && counted_metric(d,out.object,"leftMargin")==2 && counted_metric(d,out.object,"rightMargin")==2 && counted_metric(d,out.object,"blockIndent")==2);
 FVM_Path *path=0;assert(fvm_textfield_path(d,copy,&path)==FVM_OK);fvm_context_destroy(d);assert(fvm_path_count(path)>0);fvm_path_destroy(path);fvm_runtime_destroy(rt);
#endif
 return 0;
}
