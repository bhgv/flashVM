/* Forward script limits retain receiver/inputs across GC; native callers keep their barrier. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include "execution_value_callbacks.cpp"
#include "flashvm/input.h"
#include "../src/core/runtime.hpp"
static unsigned seen[4],seen_count,throw_phase,center_calls;
static FVM_Object *source_clip;
static const char *keys[]={"centerInput","xminInput","yminInput","xmaxInput","ymaxInput"};
static FVM_Result center_trap(FVM_Context *,const FVM_ScriptFrame *,FVM_Value *out,void *)
{++center_calls;fvm_value_number(out,0);return FVM_OK;}
static FVM_Result convert(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *raw)
{
 unsigned id=*(unsigned *)raw;assert(seen_count<4);seen[seen_count++]=id;FVM_Value value;fvm_value_undefined(&value);for(unsigned i=0;i<5;++i)assert(fvm_set_global(c,keys[i],&value)==FVM_OK);
 if(id==1)assert(fvm_exec_detach_clip(c,source_clip)==FVM_OK);assert(fvm_gc_collect(c,0)==(c->gc_native_depth?FVM_INVALID_ARGUMENT:FVM_OK) && fvm_object_get(c,frame->receiver,"alive",&value)==FVM_OK && value.number==17 && fvm_object_get(c,source_clip,"alive",&value)==FVM_OK && value.number==17);
 if(throw_phase==id){fvm_value_number(out,77);return fvm_exception_raise(c,out);}fvm_value_number(out,id);return FVM_OK;
}
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=4;version<=8;++version)for(unsigned native=0;native<2;++native)for(unsigned fail=0;fail<5;++fail)
 {
  FVM_Context *c=0;FVM_Object *root=0,*input;FVM_Value a[2],args[5],out,value;unsigned ids[]={1,2,3,4};unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
  assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"root",1,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK);fvm_value_string(a,"clip");fvm_value_number(a+1,1);assert(fvm_object_call(c,root,"createEmptyMovieClip",2,a,&out)==FVM_OK);source_clip=out.object;fvm_value_number(&value,17);assert(fvm_object_set(c,source_clip,"alive",&value)==FVM_OK);
  for(unsigned i=0;i<5;++i){assert(fvm_object_create(c,&input)==FVM_OK && fvm_object_set(c,input,"alive",&value)==FVM_OK);attach(c,input,"valueOf",i?convert:center_trap,i?ids+i-1:0);fvm_value_undefined(args+i);args[i].type=FVM_VALUE_OBJECT;args[i].object=input;assert(fvm_set_global(c,keys[i],args+i)==FVM_OK);}
  seen_count=center_calls=0;throw_phase=fail;FVM_GCRootFrame *roots=c->gc_frames;FVM_Result r=native?fvm_object_call(c,source_clip,"startDrag",5,args,&out):fvm_exec_drag_values(c,source_clip,args,4,args+1);
  assert(!center_calls && seen_count==(fail?fail:4) && c->gc_frames==roots && !c->gc_native_depth && !c->script_depth);for(unsigned i=0;i<seen_count;++i)assert(seen[i]==i+1);
  if(fail)assert(r==FVM_EXCEPTION && !c->drag_clip && fvm_exception_clear(c)==FVM_OK);else assert(r==FVM_OK && c->drag_clip==source_clip && c->drag_lock_center && c->drag_constrained && c->drag_bounds.x_min==1 && c->drag_bounds.y_max==4 && fvm_exec_drag_update(c)==FVM_OK && !c->drag_clip);
  assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);return 0;
}
