#include "round_curve_fixture.hpp"
int main() { FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0; assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK); ss_context(rt,&c);
 FVM_Object *clip=rc_clip(c); ss_xy(c,clip,"moveTo",100,100); FVM_Object *part=rc_part(clip); rc_number(part,"fill",1); bounds_equal(rb_bounds(c,clip),0,2,100,100); rc_number(part,"fill",0); bounds_equal(rb_bounds(c,clip),0,2,16,10);
 fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0; }
