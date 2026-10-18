/* Loaded Constant8/16 pool misses yield undefined without hiding truncated payloads. */
#define main function2_fixture_main
#include "avm1_function2.cpp"
#undef main
#include "../src/core/runtime.hpp"
static void pool(Bytes &b)
{b.push_back(0x88);u16(b,5);u16(b,1);b.push_back('o');b.push_back('k');b.push_back(0);}
static void constant(Bytes &b,bool wide,unsigned index)
{b.push_back(0x96);u16(b,wide?3:2);b.push_back(wide?9:8);b.push_back((unsigned char)index);if(wide)b.push_back((unsigned char)(index>>8));}
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned profile=0;profile<6;++profile)
 {
  FVM_Context *c=0;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)(profile+3);
  assert(fvm_context_create(rt,&c)==FVM_OK);if(profile)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
  for(unsigned populated=0;populated<2;++populated)for(unsigned wide=0;wide<2;++wide)for(unsigned which=0;which<3;++which)
  {
   unsigned index=which==0?0:which==1?1:wide?65535:255;
   Bytes b;if(populated)pool(b);constant(b,wide!=0,index);end(b);FVM_Object *fn=0;assert(fvm_function_create_avm1(c,b.begin(),b.size(),0,0,0,&fn)==FVM_OK);
   FVM_GCRootFrame *roots=c->gc_frames;FVM_Value out;FVM_Result r=fvm_function_invoke(c,fn,0,0,0,&out);
   assert(c->gc_frames==roots && !c->script_depth && !c->avm1_execution_depth);
   if(populated && !index)assert(r==FVM_OK && out.type==FVM_VALUE_STRING && !std::strcmp(out.string,"ok"));
   else if(profile)assert(r==FVM_OK && out.type==FVM_VALUE_UNDEFINED);else assert(r==FVM_INVALID_ARGUMENT);
   if(profile){Bytes sequence;if(populated)pool(sequence);constant(sequence,wide!=0,index);sequence.push_back(0x17);text(sequence,"continued");integer(sequence,7);sequence.push_back(0x1d);sequence.push_back(0);assert(fvm_avm1_execute(c,sequence.begin(),sequence.size())==FVM_OK && fvm_get_global(c,"continued",&out)==FVM_OK && out.number==7);}
   assert(fvm_gc_collect(c,0)==FVM_OK);
  }
  const unsigned char short8[]={0x96,1,0,8,0},short16[]={0x96,2,0,9,0,0};
  assert(fvm_avm1_execute(c,short8,sizeof(short8))==FVM_INVALID_ARGUMENT && fvm_avm1_execute(c,short16,sizeof(short16))==FVM_INVALID_ARGUMENT);
  fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);return 0;
}
