#include "round_bounds_fixture.hpp"
int main() {
 FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0; assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK); ss_context(rt,&c);
 for(unsigned i=0;i<2;++i) { FVM_Object *clip=rb_clip(c,i?4:0); ss_geometry(c,clip,1); double r=i?2:.5; bounds_equal(rb_bounds(c,clip),2-r,4-r,14+r,8+r); }
 fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
