#include "swf_stroke_bounds_fixture.hpp"
int main() {
 FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0; assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
 assert(fvm_context_create(rt,&c)==FVM_OK); assert(fvm_standard_install(c)==FVM_OK);
 for(unsigned type=0;type<4;++type) for(unsigned cap=0;cap<3;++cap) for(unsigned join=0;join<2;++join) {
  FVM_Object *root=load(c,bs_tags(caps_shape(type,cap,join)),1); tick(c,root); FVM_Value a[3],out;
  fvm_value_number(a,1); fvm_value_number(a+1,8); fvm_value_boolean(a+2,0);
  assert(fvm_object_call(c,root,"hitTest",3,a,&out)==FVM_OK && out.type==FVM_VALUE_BOOLEAN && !!out.boolean==(cap!=1));
  assert(fvm_object_get(c,root,"_width",&out)==FVM_OK && out.type==FVM_VALUE_NUMBER); bound_near(out.number,cap==1?12:16);
  assert(fvm_object_get(c,root,"_height",&out)==FVM_OK && out.type==FVM_VALUE_NUMBER); bound_near(out.number,4);
  fvm_value_number(a,cap==1?24:32); assert(fvm_object_set(c,root,"_width",a)==FVM_OK);
  assert(fvm_object_get(c,root,"_width",&out)==FVM_OK && out.type==FVM_VALUE_NUMBER); bound_near(out.number,cap==1?24:32);
 }
 fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
