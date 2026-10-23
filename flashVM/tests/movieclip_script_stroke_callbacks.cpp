#include "script_stroke_fixture.hpp"
static unsigned calls,fail_at;
static bool mutate_fill;
static FVM_Object *target;
static FVM_Result option(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *userdata) {
 ++calls; assert(c->gc_native_depth && fvm_gc_collect(c,0)==FVM_INVALID_ARGUMENT); if(fail_at==calls) return FVM_ERROR; unsigned kind=(unsigned)(size_t)userdata;
 if(kind<3) { fvm_value_number(out,kind==0?4:kind==1?0xff0000:100); return FVM_OK; }
 if(kind==3 && mutate_fill) { FVM_Value color,ignored; fvm_value_number(&color,0x0000ff); assert(fvm_object_call(c,target,"beginFill",1,&color,&ignored)==FVM_OK); }
 return fvm_value_string(out,kind==3?"normal":kind==4?"square":"bevel"); }
int main() { FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0; assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK); ss_context(rt,&c); target=ss_clip(c,1,0); FVM_Value a[8],out; ss_args(a,0,0);
 for(unsigned i=0;i<6;++i) { FVM_Object *object=0; assert(fvm_object_create(c,&object)==FVM_OK && fvm_gc_pin(c,object)==FVM_OK); assert(fvm_object_define_method_rooted(c,object,i<3?"valueOf":"toString",option,(void*)(size_t)i,0)==FVM_OK); ref(a+(i<3?i:i+1),object); }
 for(unsigned failure=1;failure<=6;++failure) { calls=0; fail_at=failure; FVM_Object *active=target->resource_result.object->properties["active"].object; assert(fvm_object_call(c,target,"lineStyle",7,a,&out)==FVM_ERROR && calls==failure); assert(target->resource_result.object->properties["active"].object==active && !c->gc_frames && !c->gc_native_depth && !c->script_depth); }
 calls=fail_at=0; mutate_fill=true; assert(fvm_object_call(c,target,"lineStyle",7,a,&out)==FVM_OK && calls==6); mutate_fill=false;
 assert(target->resource_result.object->properties["fill"].number==1 && target->resource_result.object->properties["rgb"].number==255); assert(fvm_gc_collect(c,0)==FVM_OK); ss_geometry(c,target);
 FVM_Object *part=target->resource_result.object->properties["paths"].object->indexed[1].object; assert(part->properties["lineCap"].number==2 && part->properties["lineJoin"].number==1);
 fvm_value_undefined(a); calls=0; assert(fvm_object_call(c,target,"lineStyle",7,a,&out)==FVM_OK && calls==0);
 fvm_context_destroy(c); ss_context(rt,&c,7); target=ss_clip(c,0,0); ss_args(a,0,0); for(unsigned i=3;i<7;++i) { FVM_Object *object=0; assert(fvm_object_create(c,&object)==FVM_OK && fvm_gc_pin(c,object)==FVM_OK); assert(fvm_object_define_method_rooted(c,object,"toString",option,(void*)3,0)==FVM_OK); ref(a+i,object); } calls=0; fail_at=1; assert(fvm_object_call(c,target,"lineStyle",7,a,&out)==FVM_OK && calls==0);
 fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0; }
