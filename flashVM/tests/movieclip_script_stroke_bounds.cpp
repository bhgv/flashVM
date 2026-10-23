#include "script_stroke_fixture.hpp"
int main() {
 FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0; assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK); ss_context(rt,&c);
 for(unsigned cap=0;cap<3;++cap) for(unsigned join=0;join<2;++join) { FVM_Object *clip=ss_clip(c,cap,join); FVM_Value a[3],out; ref(a,clip); assert(fvm_object_call(c,clip,"getBounds",1,a,&out)==FVM_OK); FVM_Object *b=out.object;
  bound_near(b->properties["xMin"].number,cap==1?2:0); bound_near(b->properties["xMax"].number,cap==1?14:16); bound_near(b->properties["yMin"].number,6); bound_near(b->properties["yMax"].number,10);
  fvm_value_number(a,.1); fvm_value_number(a+1,6.1); fvm_value_boolean(a+2,1); assert(fvm_object_call(c,clip,"hitTest",3,a,&out)==FVM_OK && !!out.boolean==(cap==2));
 }
 fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
