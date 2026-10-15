/* Clone coercions collect in depth/name/source order and preserve all explicit operands. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include "execution_value_callbacks.cpp"
#include "../src/core/runtime.hpp"
static unsigned clone_order[3],clone_calls,clone_phase;
static FVM_Result clone_coercion(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *raw)
{
 unsigned id=*(unsigned *)raw;assert(clone_calls<3);clone_order[clone_calls++]=id;
 FVM_Value v;fvm_value_undefined(&v);assert(fvm_set_global(c,"sourceInput",&v)==FVM_OK && fvm_set_global(c,"nameInput",&v)==FVM_OK && fvm_set_global(c,"depthInput",&v)==FVM_OK && fvm_gc_collect(c,0)==FVM_OK && fvm_object_get(c,frame->receiver,"alive",&v)==FVM_OK && v.number==17);
 if(clone_phase==id){fvm_value_number(out,77);return fvm_exception_raise(c,out);}if(id==1){fvm_value_number(out,16386);return FVM_OK;}return fvm_value_string(out,id==2?"copy":"source");
}
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=4;version<=8;++version)for(unsigned interpreter=0;interpreter<2;++interpreter)for(unsigned fail=0;fail<4;++fail)
 {
  FVM_Context *c=0;FVM_Object *root=0,*objects[3];FVM_Value a[2],inputs[3],out,v;unsigned ids[]={3,2,1};const char *keys[]={"sourceInput","nameInput","depthInput"};unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
  assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"root",1,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK);
  fvm_value_string(a,"source");fvm_value_number(a+1,1);assert(fvm_object_call(c,root,"createEmptyMovieClip",2,a,&out)==FVM_OK);
  for(unsigned i=0;i<3;++i){assert(fvm_object_create(c,objects+i)==FVM_OK);fvm_value_number(&v,17);assert(fvm_object_set(c,objects[i],"alive",&v)==FVM_OK);attach(c,objects[i],i==2?"valueOf":"toString",clone_coercion,ids+i);fvm_value_undefined(inputs+i);inputs[i].type=FVM_VALUE_OBJECT;inputs[i].object=objects[i];assert(fvm_set_global(c,keys[i],inputs+i)==FVM_OK);}
  Bytes b;for(unsigned i=0;i<3;++i){text(b,keys[i]);b.push_back(0x1c);}b.push_back(0x24);b.push_back(0);
  clone_calls=0;clone_phase=fail;FVM_GCRootFrame *roots=c->gc_frames;
  FVM_Result r=interpreter?fvm_avm1_execute_receiver(c,b.begin(),b.size(),root):fvm_exec_duplicate_target(c,root,inputs,inputs+1,inputs+2,inputs+1);
  assert(clone_calls==(fail?fail:3) && c->gc_frames==roots && !c->gc_native_depth && !c->script_depth && !c->avm1_target_active && !c->avm1_execution_depth);for(unsigned i=0;i<clone_calls;++i)assert(clone_order[i]==i+1);
  if(fail){assert(r==FVM_EXCEPTION && inputs[1].type==FVM_VALUE_OBJECT && inputs[1].object==objects[1] && fvm_exception_clear(c)==FVM_OK);}
  else{assert(r==FVM_OK && fvm_object_get(c,root,"copy",&out)==FVM_OK && out.type==FVM_VALUE_OBJECT);if(!interpreter)assert(inputs[1].object==out.object);}
  fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);return 0;
}
