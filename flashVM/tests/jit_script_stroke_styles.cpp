#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#include "script_stroke_code_fixture.hpp"
int main() { if(!fvm_jit_available()) return 0; FVM_Config cfg; FVM_Runtime *rt=0; assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=7;version<=8;++version) for(unsigned cap=0;cap<3;++cap) for(unsigned join=0;join<2;++join) for(class_mode=0;class_mode<3;++class_mode) {
  FVM_Context *c=0; ss_context(rt,&c,version); FVM_Object *clip=0; assert(fvm_movieclip_create(c,"root",1,&clip)==FVM_OK && fvm_gc_pin(c,clip)==FVM_OK); FVM_Value v; ref(&v,clip); assert(fvm_set_global(c,"paintClip",&v)==FVM_OK); Program program; program.bytes=ss_code(cap,join); program.bytes.pop_back(); program.number(42); program.finish(); FVM_Value result=class_run(c,program); assert(result.type==FVM_VALUE_NUMBER && result.number==42);
  ss_metadata(clip,version==8?cap:0,version==8?join:0); ss_pixels(c,clip,version==8?cap:0,version==8?join:0); fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt); return 0; }
