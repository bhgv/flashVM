#include "script_stroke_fixture.hpp"
int main() {
 FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0; assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK); ss_context(rt,&c);
 for(unsigned cap=0;cap<3;++cap) for(unsigned join=0;join<2;++join) {
  FVM_Object *clip=ss_clip(c,cap,join); ss_metadata(clip,cap,join); ss_pixels(c,clip,cap,join); assert(fvm_gc_collect(c,0)==FVM_OK); ss_pixels(c,clip,cap,join);
 }
 fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
