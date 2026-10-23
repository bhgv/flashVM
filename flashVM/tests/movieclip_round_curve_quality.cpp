#include "round_curve_fixture.hpp"
int main() { FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0; assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK); ss_context(rt,&c);
 for(unsigned kind=0;kind<4;++kind) { FVM_Object *clip=rc_clip(c,4,kind); ss_xy(c,clip,"moveTo",100,100); for(unsigned q=0;q<4;++q) { assert(fvm_stage_set_quality(c,(FVM_StageQuality)q)==FVM_OK); bounds_equal(rb_bounds(c,clip),0,2,16,10); } }
 fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0; }
