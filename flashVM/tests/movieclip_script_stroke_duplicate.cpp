#include "script_stroke_fixture.hpp"
int main() {
 FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0; assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK); ss_context(rt,&c);
 FVM_Object *root=0; assert(fvm_movieclip_create(c,"root",1,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK);
 for(unsigned cap=0;cap<3;++cap) for(unsigned join=0;join<2;++join) { FVM_Object *clip=new_child(c,root,"source",1); ss_style(c,clip,cap,join); ss_geometry(c,clip); FVM_Value a[2],out; assert(fvm_value_string(a,"copy")==FVM_OK); fvm_value_number(a+1,2); assert(fvm_object_call(c,clip,"duplicateMovieClip",2,a,&out)==FVM_OK); FVM_Object *copy=out.object;
  ss_metadata(copy,cap,join); ss_style(c,clip,0,0); ss_metadata(copy,cap,join); ss_pixels(c,copy,cap,join); assert(fvm_gc_collect(c,0)==FVM_OK);
 }
 fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
