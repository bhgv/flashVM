#include "script_stroke_code_fixture.hpp"
int main() { FVM_Config cfg; FVM_Runtime *rt=0; assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=6;version<=8;++version) for(unsigned cap=0;cap<3;++cap) for(unsigned join=0;join<2;++join) { FVM_Context *c=0; ss_context(rt,&c,version); FVM_Object *clip=0; assert(fvm_movieclip_create(c,"root",1,&clip)==FVM_OK && fvm_gc_pin(c,clip)==FVM_OK); FVM_Value v; ref(&v,clip); assert(fvm_set_global(c,"paintClip",&v)==FVM_OK); Bytes code=ss_code(cap,join); assert(fvm_avm1_execute(c,code.begin(),code.size())==FVM_OK); ss_metadata(clip,version==8?cap:0,version==8?join:0); ss_pixels(c,clip,version==8?cap:0,version==8?join:0); fvm_context_destroy(c); }
 fvm_runtime_destroy(rt); return 0; }
