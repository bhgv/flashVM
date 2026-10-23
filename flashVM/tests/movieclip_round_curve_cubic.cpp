#include "round_curve_fixture.hpp"
int main() { FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0; assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK); ss_context(rt,&c);
 FVM_Object *clip=rc_clip(c); rc_cubic(clip); FVM_Value a,out; ref(&a,clip); assert(fvm_object_call(c,clip,"getBounds",1,&a,&out)==FVM_INVALID_ARGUMENT); FVM_Object *command=rc_part(clip)->properties["commands"].object->indexed[1].object; rc_number(command,"verb",FVM_PATH_QUADRATIC); rc_number(command,"x0",8); rc_number(command,"y0",0); rc_number(command,"x1",14); rc_number(command,"y1",8); ss_xy(c,clip,"moveTo",100,100); bounds_equal(rb_bounds(c,clip),0,2,16,10);
 fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0; }
