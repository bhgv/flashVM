/* Font variants, tracking, underline and empty whitespace use the same expanded advance. */
#define FVM_FONT_FACES_FIXTURE_ONLY
#include "textfield_font_faces.cpp"
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned variant=0;variant<4;++variant){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);FVM_Object *field=counted_field(c),*format=0;register_styles(c);FVM_Value v,arg,out;assert(fvm_construct(c,"TextFormat",0,0,&format)==FVM_OK);select_style(c,format,variant);
  fvm_value_number(&v,40);assert(fvm_object_set(c,format,"size",&v)==FVM_OK);fvm_value_number(&v,1);assert(fvm_object_set(c,format,"letterSpacing",&v)==FVM_OK);assert(fvm_value_string(&v,"justify")==FVM_OK && fvm_object_set(c,format,"align",&v)==FVM_OK);
  fvm_value_boolean(&v,1);assert(fvm_object_set(c,format,"underline",&v)==FVM_OK && fvm_object_set(c,field,"multiline",&v)==FVM_OK && fvm_object_set(c,field,"wordWrap",&v)==FVM_OK);fvm_value_number(&v,94);assert(fvm_object_set(c,field,"_width",&v)==FVM_OK);
  fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=format;assert(fvm_object_call(c,field,"setNewTextFormat",1,&arg,&out)==FVM_OK && fvm_value_string(&v,"A V A V")==FVM_OK && fvm_object_set(c,field,"text",&v)==FVM_OK && fvm_value_string(&v,"left")==FVM_OK && fvm_object_set(c,field,"autoSize",&v)==FVM_OK && counted_metric(c,field,"textWidth")==90);
  FVM_Path *path=0;int hit=0;assert(fvm_textfield_path(c,field,&path)==FVM_OK && fvm_path_hit_test(path,91,42,0,1,&hit)==FVM_OK && hit);fvm_path_destroy(path);
  assert(fvm_value_string(&v,"                 ")==FVM_OK && fvm_object_set(c,field,"text",&v)==FVM_OK && counted_metric(c,field,"textWidth")==0 && counted_metric(c,field,"_height")==48);
  assert(fvm_gc_collect(c,0)==FVM_OK && fvm_textfield_path(c,field,&path)==FVM_OK && fvm_path_count(path)==0);fvm_path_destroy(path);fvm_context_destroy(c);
 }fvm_runtime_destroy(rt);
#endif
 return 0;
}
