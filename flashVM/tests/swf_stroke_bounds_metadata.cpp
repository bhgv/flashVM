#include "swf_stroke_bounds_fixture.hpp"
int main() {
 FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0; assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
 assert(fvm_context_create(rt,&c)==FVM_OK); assert(fvm_standard_install(c)==FVM_OK);
 for(unsigned type=0;type<4;++type) {
  FVM_Object *root=load(c,bs_tags(caps_shape(type,2,1)),1); tick(c,root); FVM_Object *p=bounds_part(root);
  const char *names[]={"lineCap","lineJoin","width"};
  for(unsigned style=0;style<3;++style) { FVM_Value prior=p->properties[names[style]];
   const double values[]={-1,HUGE_VAL,HUGE_VAL-HUGE_VAL};
   for(unsigned i=0;i<3;++i) { bounds_number(p,names[style],values[i]); FVM_PathBounds b={1,2,3,4}; assert(clip_bounds(c,root,&b)==FVM_INVALID_ARGUMENT); bounds_equal(b,1,2,3,4); assert(!c->gc_frames && !c->gc_native_depth && !c->script_depth); }
   p->properties[names[style]]=prior;
  }
  selected_bounds(c,root,2,1); p->properties.erase("lineCap"); p->properties.erase("lineJoin"); selected_bounds(c,root,0,0);
 }
 fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
