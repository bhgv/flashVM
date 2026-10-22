/* All alignments place glyphs and real-face underline inside the same inset box. */
#define FVM_INSETS_FIXTURE_ONLY
#include "textfield_insets_fixture.hpp"
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);register_styles(c);
 FVM_Object *field=counted_field(c),*format=inset_format(c);inset_number(c,field,"_width",124);inset_bool(c,format,"underline",true);
 const char *align[]={"left","center","right","justify"};const double advances[]={60,68,64,72};
 for(unsigned face=0;face<4;++face){select_style(c,format,face);
  for(unsigned a=0;a<4;++a){inset_string(c,format,"align",align[a]);inset_apply(c,field,format,"setNewTextFormat");inset_string(c,field,"text","A V");double origin=20+(a==1?(90-advances[face])/2:a==2?90-advances[face]:0);FVM_Path *path=0;assert(fvm_textfield_path(c,field,&path)==FVM_OK);FVM_PathBounds b;assert(fvm_path_bounds(path,&b)==FVM_OK && std::fabs(b.x_min-origin)<0.02);int hit=0;assert(fvm_path_hit_test(path,origin+advances[face]-1,42,0,1,&hit)==FVM_OK && hit);fvm_path_destroy(path);}
 }
 select_style(c,format,0);inset_string(c,format,"align","justify");inset_bool(c,field,"multiline",true);inset_bool(c,field,"wordWrap",true);inset_apply(c,field,format,"setNewTextFormat");inset_string(c,field,"text","A V A V");assert(counted_metric(c,field,"textWidth")==90);
 FVM_Path *path=0;assert(fvm_textfield_path(c,field,&path)==FVM_OK);int hit=0;assert(fvm_path_hit_test(path,109,42,0,1,&hit)==FVM_OK && hit);fvm_context_destroy(c);assert(fvm_path_count(path)>0);fvm_path_destroy(path);fvm_runtime_destroy(rt);
#endif
 return 0;
}
