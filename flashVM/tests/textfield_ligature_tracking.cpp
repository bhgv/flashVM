/* Field auto-size and transfer use decomposed glyphs while retaining future formatting. */
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include "textfield_counted_text.cpp"
#include <cmath>
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0,*d=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_context_create(rt,&d)==FVM_OK);
 FVM_Object *field=counted_field(c),*format=0;FVM_Value arg,v,out,root;
 assert(fvm_construct(c,"TextFormat",0,0,&format)==FVM_OK);fvm_value_number(&v,40);assert(fvm_object_set(c,format,"size",&v)==FVM_OK);fvm_value_number(&v,1);assert(fvm_object_set(c,format,"letterSpacing",&v)==FVM_OK);
 fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=format;assert(fvm_object_call(c,field,"setNewTextFormat",1,&arg,&out)==FVM_OK);
 assert(fvm_value_string(&v,"fi")==FVM_OK && fvm_object_set(c,field,"text",&v)==FVM_OK && counted_metric(c,field,"textWidth")==26);
 assert(fvm_value_string(&v,"center")==FVM_OK && fvm_object_set(c,field,"autoSize",&v)==FVM_OK && counted_metric(c,field,"_width")==30 && counted_metric(c,field,"_x")==35);
 FVM_Value extent;assert(fvm_value_string(&v,"fi")==FVM_OK && fvm_object_call(c,format,"getTextExtent",1,&v,&extent)==FVM_OK && counted_metric(c,extent.object,"width")==26 && counted_metric(c,extent.object,"textFieldWidth")==30);
 fvm_value_number(&v,20);assert(fvm_object_set(c,format,"size",&v)==FVM_OK);fvm_value_number(&v,-1);assert(fvm_object_set(c,format,"letterSpacing",&v)==FVM_OK && fvm_object_call(c,field,"setNewTextFormat",1,&arg,&out)==FVM_OK);
 fvm_value_undefined(&root);root.type=FVM_VALUE_OBJECT;root.object=field;unsigned char *wire=0;size_t size=0;assert(fvm_standard_text_install(d)==FVM_OK && fvm_value_serialize(c,&root,&wire,&size)==FVM_OK && fvm_value_deserialize(d,wire,size,&out)==FVM_OK);fvm_serialized_free(wire);
 FVM_Object *copy=out.object;assert(fvm_gc_pin(d,copy)==FVM_OK && counted_metric(d,copy,"textWidth")==26 && counted_metric(d,copy,"_x")==35);
 FVM_Channel *ch=0;assert(fvm_channel_create(&ch)==FVM_OK && fvm_channel_send_value(c,ch,&root,0)==FVM_OK && fvm_channel_receive_value(d,ch,&out,0)==FVM_OK);fvm_channel_destroy(ch);assert(counted_metric(d,out.object,"textWidth")==26);
 fvm_context_destroy(c);assert(fvm_gc_collect(d,0)==FVM_OK);
 assert(fvm_value_string(&v,"fifi")==FVM_OK && fvm_object_set(d,copy,"text",&v)==FVM_OK && counted_metric(d,copy,"textWidth")==20 && counted_metric(d,copy,"_width")==24 && counted_metric(d,copy,"_x")==38);
 FVM_Path *path=0;assert(fvm_textfield_path(d,copy,&path)==FVM_OK);fvm_context_destroy(d);assert(fvm_path_count(path)>0);fvm_path_destroy(path);fvm_runtime_destroy(rt);
#endif
 return 0;
}
