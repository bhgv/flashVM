#include "script_stroke_fixture.hpp"
int main() { FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0; assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK); ss_context(rt,&c);
 for(unsigned cap=0;cap<3;++cap) for(unsigned join=0;join<2;++join) { FVM_Object *clip=0; assert(fvm_movieclip_create(c,"mixed",1,&clip)==FVM_OK && fvm_gc_pin(c,clip)==FVM_OK); FVM_Value a[8],out; spread_arguments(c,a,1); assert(fvm_object_call(c,clip,"beginGradientFill",8,a,&out)==FVM_OK); FVM_Object *fill=clip->resource_result.object->properties["gradient"].object;
  ss_style(c,clip,cap,join); assert(clip->resource_result.object->properties["gradient"].object==fill); ss_geometry(c,clip); FVM_Object *part=clip->resource_result.object->properties["active"].object; assert(part->properties["gradient"].object==fill && part->properties["fill"].number==1); ss_metadata(clip,cap,join);
  assert(fvm_object_call(c,clip,"endFill",0,0,&out)==FVM_OK); ss_xy(c,clip,"moveTo",2,12); ss_xy(c,clip,"lineTo",14,12); part=clip->resource_result.object->properties["active"].object; if(cap) assert(part->properties["lineCap"].number==cap); if(join) assert(part->properties["lineJoin"].number==join); assert(part->properties["fill"].number==0);
 }
 fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0; }
