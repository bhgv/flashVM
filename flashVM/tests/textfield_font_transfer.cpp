/* Selected current/future faces cross binary/channel boundaries without regular-face substitution. */
#define FVM_FONT_FACES_FIXTURE_ONLY
#include "textfield_font_faces.cpp"
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0,*d=0,*bad=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_context_create(rt,&d)==FVM_OK && fvm_context_create(rt,&bad)==FVM_OK);
 FVM_Object *field=counted_field(c),*format=0;register_styles(c);FVM_Value v,arg,out,root;assert(fvm_construct(c,"TextFormat",0,0,&format)==FVM_OK);
 fvm_value_number(&v,40);assert(fvm_object_set(c,format,"size",&v)==FVM_OK);select_style(c,format,FVM_TEXT_FONT_BOLD);fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=format;
 assert(fvm_object_call(c,field,"setNewTextFormat",1,&arg,&out)==FVM_OK && fvm_value_string(&v,"AV")==FVM_OK && fvm_object_set(c,field,"text",&v)==FVM_OK);
 assert(fvm_value_string(&v,"center")==FVM_OK && fvm_object_set(c,field,"autoSize",&v)==FVM_OK && counted_metric(c,field,"_width")==60 && counted_metric(c,field,"_x")==20);
 select_style(c,format,FVM_TEXT_FONT_ITALIC);fvm_value_number(&v,20);assert(fvm_object_set(c,format,"size",&v)==FVM_OK && fvm_object_call(c,field,"setNewTextFormat",1,&arg,&out)==FVM_OK);
 fvm_value_undefined(&root);root.type=FVM_VALUE_OBJECT;root.object=field;unsigned char *wire=0;size_t size=0;assert(fvm_standard_text_install(d)==FVM_OK && fvm_value_serialize(c,&root,&wire,&size)==FVM_OK && fvm_value_deserialize(d,wire,size,&out)==FVM_OK);
 FVM_Object *copy=out.object;assert(fvm_gc_pin(d,copy)==FVM_OK && d->fonts.size()==2 && counted_metric(d,copy,"textWidth")==56);
 assert(fvm_object_call(d,copy,"getTextFormat",0,0,&out)==FVM_OK && fvm_object_get(d,out.object,"bold",&v)==FVM_OK && v.boolean);
 assert(fvm_object_call(d,copy,"getNewTextFormat",0,0,&out)==FVM_OK && fvm_object_get(d,out.object,"italic",&v)==FVM_OK && v.boolean);
 /* A conflicting future slot rejects the graph and rolls back the earlier current-face import. */
 assert(fvm_standard_text_install(bad)==FVM_OK && fvm_standard_text_register_font_variant(bad,"Geometry",FVM_TEXT_FONT_ITALIC,fvm_test_font,sizeof(fvm_test_font),0)==FVM_OK);
 assert(fvm_value_deserialize(bad,wire,size,&out)==FVM_INVALID_ARGUMENT && bad->fonts.size()==1 && bad->default_font.empty());
 assert(fvm_standard_text_register_font_variant(bad,"Geometry",FVM_TEXT_FONT_BOLD,fvm_test_font_bold,sizeof(fvm_test_font_bold),0)==FVM_OK);fvm_context_destroy(bad);fvm_serialized_free(wire);
 FVM_Channel *ch=0;assert(fvm_channel_create(&ch)==FVM_OK && fvm_channel_send_value(c,ch,&root,0)==FVM_OK && fvm_channel_receive_value(d,ch,&out,0)==FVM_OK);fvm_channel_destroy(ch);assert(counted_metric(d,out.object,"textWidth")==56);
 fvm_context_destroy(c);assert(fvm_gc_collect(d,0)==FVM_OK);
 assert(fvm_value_string(&v,"AVA")==FVM_OK && fvm_object_set(d,copy,"text",&v)==FVM_OK && counted_metric(d,copy,"textWidth")==39 && counted_metric(d,copy,"_width")==43 && counted_metric(d,copy,"_x")==28.5);
 FVM_Object *plain=0;assert(fvm_construct(d,"TextFormat",0,0,&plain)==FVM_OK);select_style(d,plain,FVM_TEXT_FONT_REGULAR);arg.object=plain;assert(fvm_object_call(d,copy,"setTextFormat",1,&arg,&out)==FVM_NOT_FOUND && counted_metric(d,copy,"textWidth")==39);
 FVM_Path *path=0;assert(fvm_textfield_path(d,copy,&path)==FVM_OK);fvm_context_destroy(d);assert(fvm_path_count(path)>0);fvm_path_destroy(path);fvm_runtime_destroy(rt);
#endif
 return 0;
}
