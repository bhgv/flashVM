#include "swf_stroke_bounds_fixture.hpp"
int main() {
 FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0; assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
 assert(fvm_context_create(rt,&c)==FVM_OK); assert(fvm_standard_install(c)==FVM_OK);
 for(unsigned type=0;type<4;++type) for(unsigned cap=0;cap<3;++cap) for(unsigned join=0;join<2;++join) {
  FVM_Object *root=load(c,bs_tags(caps_shape(type,cap,join)),1); tick(c,root); FVM_Object *part=bounds_part(root),*commands=part->properties["commands"].object,*move=0;
  assert(fvm_object_create(c,&move)==FVM_OK); bounds_number(move,"verb",FVM_PATH_MOVE); bounds_number(move,"x0",100); bounds_number(move,"y0",100);
  FVM_Value v; fvm_value_undefined(&v); v.type=FVM_VALUE_OBJECT; v.object=move; commands->indexed[2]=v;
  selected_bounds(c,root,cap,join); bounds_number(part,"fill",1); FVM_PathBounds b; assert(clip_bounds(c,root,&b)==FVM_OK); bounds_equal(b,cap==1?2:0,6,100,100);
  bounds_number(part,"fill",0); commands->indexed.erase(2); FVM_Object *line=commands->indexed[1].object; bounds_number(line,"x0",2);
  assert(clip_bounds(c,root,&b)==FVM_OK); bounds_equal(b,0,0,0,0);
  bounds_number(part,"fill",1); assert(clip_bounds(c,root,&b)==FVM_OK); bounds_equal(b,2,8,2,8);
 }
 fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
