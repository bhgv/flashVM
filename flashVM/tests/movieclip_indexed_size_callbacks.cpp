/* Geometry traversal snapshots survive reentrant removal and callback errors. */
#define FVM_INDEXED_SIZE_FIXTURE_ONLY
#include "movieclip_indexed_size.cpp"
#include "flashvm/property.h"
#include "flashvm/function.h"
#include "flashvm/exception.h"
static FVM_Object *removed;
static unsigned calls;
static bool fail;
static FVM_Result transform_get(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{
 assert(c->gc_native_depth && fvm_gc_collect(c,0)==FVM_INVALID_ARGUMENT);
 if(!calls++)assert(fvm_exec_detach_clip(c,removed)==FVM_OK);
 if(fail)return FVM_ERROR;
 fvm_value_number(out,0);return FVM_OK;
}
static FVM_Result forbidden_size(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *,void *)
{ assert(false);return FVM_ERROR; }
static FVM_Object *coercion_clip;
static unsigned coercion_failure;
static FVM_Result collect_index(FVM_Context *c,const FVM_ScriptFrame *,FVM_Value *out,void *)
{
 assert(!c->gc_native_depth && fvm_exec_detach_clip(c,coercion_clip)==FVM_OK);
 FVM_Object *garbage=0;assert(fvm_object_create(c,&garbage)==FVM_OK);
 unsigned long reclaimed=0;assert(fvm_gc_collect(c,&reclaimed)==FVM_OK && reclaimed>0);
 if(coercion_failure==1){fvm_value_number(out,81);return fvm_exception_raise(c,out);}
 if(coercion_failure==2)return FVM_ERROR;
 fvm_value_number(out,8);return FVM_OK;
}
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=4;version<=8;++version) {
  FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);
  unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
  assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
  FVM_Object *root=0;assert(fvm_movieclip_create(c,"root",1,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK);
  FVM_Object *first=child(c,root,"first",1);removed=child(c,root,"removed",2);
  size_rect(c,first,0,0,10,20);size_rect(c,removed,100,0,5,5);
  assert(fvm_object_define_accessor_rooted(c,root,"_width",forbidden_size,0,0,0)==FVM_OK);
  assert(fvm_object_define_accessor_rooted(c,root,"_height",forbidden_size,0,0,0)==FVM_OK);
  int deleted=0;assert(fvm_object_delete(c,first,"_x",&deleted)==FVM_OK && deleted);
  assert(fvm_object_define_accessor_rooted(c,first,"_x",transform_get,0,0,0)==FVM_OK);
  calls=0;fail=false;size_expect(c,root,8,105);assert(calls==1);
  assert(fvm_gc_collect(c,0)==FVM_OK);size_expect(c,root,8,10);size_expect(c,root,9,20);
  fail=true;FVM_Value target=size_object(root),at,out;fvm_value_number(&at,8);fvm_value_number(&out,77);
  assert(fvm_exec_display_property(c,0,&target,&at,0,&out)==FVM_ERROR && out.number==77);
  assert(c->gc_frames==0 && c->gc_native_depth==0);fail=false;size_expect(c,root,8,10);
  fvm_context_destroy(c);
 }
 for(unsigned version=4;version<=8;++version)for(coercion_failure=0;coercion_failure<3;++coercion_failure) {
  FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);
  unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
  assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
  FVM_Object *root=0;assert(fvm_movieclip_create(c,"root",1,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK);
  coercion_clip=child(c,root,"clip",1);size_rect(c,coercion_clip,0,0,10,20);
  FVM_Object *index=0,*fn=0;const unsigned char code[]={0};
  assert(fvm_construct(c,"Object",0,0,&index)==FVM_OK);
  assert(fvm_function_create_avm1(c,code,sizeof(code),0,0,0,&fn)==FVM_OK && fvm_function_set_body(c,fn,collect_index,0,0)==FVM_OK);
  FVM_Value value=size_object(fn);assert(fvm_object_set(c,index,"valueOf",&value)==FVM_OK);
  FVM_Value target=size_object(coercion_clip),at=size_object(index);FVM_Result r=fvm_exec_display_property(c,0,&target,&at,0,&target);
  assert(c->gc_frames==0 && c->gc_native_depth==0 && !c->script_depth);
  if(!coercion_failure)assert(r==FVM_OK && target.type==FVM_VALUE_NUMBER && target.number==10);
  else {
   assert(r==(coercion_failure==1?FVM_EXCEPTION:FVM_ERROR) && target.type==FVM_VALUE_OBJECT && target.object==coercion_clip);
   if(coercion_failure==1)assert(fvm_exception_get(c,&value)==FVM_OK && value.number==81 && fvm_exception_clear(c)==FVM_OK);
   assert(fvm_gc_pin(c,coercion_clip)==FVM_OK);size_expect(c,coercion_clip,9,20);
  }
  fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);
}
