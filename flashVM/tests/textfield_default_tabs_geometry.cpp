/* Default interval follows the selected real face; inset/indent/alignment do not double-count it. */
#include "textfield_tabs_fixture.hpp"
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);
 FVM_Object *field=counted_field(c);unsigned char bold[sizeof(fvm_test_font_bold)];std::memcpy(bold,fvm_test_font_bold,sizeof(bold));
 /* Fixture OS/2 is independently at 328. Override only average=1000 units/em. */
 assert(bold[328]==0 && bold[329]==3 && bold[330]==2 && bold[331]==35);bold[330]=3;bold[331]=232;
 assert(fvm_standard_text_register_font_variant(c,"Geometry",FVM_TEXT_FONT_BOLD,bold,sizeof(bold),0)==FVM_OK);
 const char *alignments[]={"left","center","right"};
 for(unsigned which=0;which<3;++which){FVM_Object *format=tabs_format(c,0,0);select_style(c,format,FVM_TEXT_FONT_BOLD);inset_number(c,format,"leftMargin",10);inset_number(c,format,"rightMargin",12);inset_number(c,format,"blockIndent",8);inset_number(c,format,"indent",-10);inset_number(c,format,"letterSpacing",0.5);inset_string(c,format,"align",alignments[which]);inset_apply(c,field,format,"setNewTextFormat");inset_string(c,field,"text","A\tV");inset_string(c,field,"autoSize","left");
  assert(counted_metric(c,field,"textWidth")==188.5 && counted_metric(c,field,"_width")==222.5);
  FVM_Path *path=0;assert(fvm_textfield_path(c,field,&path)==FVM_OK && fvm_path_count(path)>0);fvm_path_destroy(path);
  inset_number(c,format,"size",20);inset_apply(c,field,format,"setTextFormat");assert(counted_metric(c,field,"textWidth")==94.5 && counted_metric(c,field,"_width")==128.5);
 }
 assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth);fvm_context_destroy(c);fvm_runtime_destroy(rt);
#endif
 return 0;
}
