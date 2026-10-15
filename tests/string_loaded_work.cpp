/* Loaded split/search traversal shares execution work and permits large split results. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
#include "../src/core/runtime.hpp"
#include <limits>
static void string_work_load(FVM_Context *c,unsigned version)
{if(version){unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);}assert(fvm_standard_install(c)==FVM_OK);}
static FVM_Object *string_work_make(FVM_Context *c,const char *text,size_t size)
{FVM_Value value;FVM_Object *str=0;assert(fvm_value_string_bytes(&value,text,size)==FVM_OK && fvm_construct(c,"String",1,&value,&str)==FVM_OK && fvm_gc_pin(c,str)==FVM_OK);return str;}
static void string_work_clean(FVM_Context *c)
{assert(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth && !c->string_depth && !c->primitive_depth);}
#ifndef FVM_STRING_WORK_FIXTURE_ONLY
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=0;version<=8;++version) {
  if(version && version<4)continue;FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);string_work_load(c,version);
  fvm_detail::vector<char> input;for(unsigned i=0;i<65537;++i){if(i)input.push_back(',');input.push_back('x');}
  FVM_Object *str=string_work_make(c,input.begin(),input.size());FVM_Value args[2],out,value;fvm_value_string(args,",");
  FVM_Result r=fvm_object_call(c,str,"split",1,args,&out);
  if(!version)assert(r==FVM_OUT_OF_MEMORY);else{
   assert(r==FVM_OK && fvm_gc_pin(c,out.object)==FVM_OK);number(c,out.object,"length",65537);
   assert(fvm_gc_collect(c,0)==FVM_OK && fvm_table_get_index(c,out.object,65536,&value)==FVM_OK && value.type==FVM_VALUE_STRING && !std::strcmp(value.string,"x"));
   assert(fvm_gc_unpin(c,out.object)==FVM_OK);
  }
  unsigned long limit=rt->config.instruction_limit;rt->config.instruction_limit=8;
  r=fvm_object_call(c,str,"split",1,args,&out);assert(r==(version?FVM_ERROR:FVM_OUT_OF_MEMORY));if(version)assert(c->avm1_steps==8);string_work_clean(c);
  rt->config.instruction_limit=0;fvm_value_number(args+1,3);assert(fvm_object_call(c,str,"split",2,args,&out)==FVM_OK);number(c,out.object,"length",3);
  fvm_detail::vector<char> hay,needle;for(unsigned i=0;i<4096;++i)hay.push_back('a');hay.push_back('b');for(unsigned i=0;i<1024;++i)needle.push_back('a');needle.push_back('b');
  FVM_Object *search=string_work_make(c,hay.begin(),hay.size());assert(fvm_value_string_bytes(args,needle.begin(),needle.size())==FVM_OK);
  const char *methods[]={"indexOf","lastIndexOf"};
  for(unsigned i=0;i<2;++i){assert(fvm_object_call(c,search,methods[i],1,args,&out)==FVM_OK && out.number==3072);rt->config.instruction_limit=64;
   assert(fvm_object_call(c,search,methods[i],1,args,&out)==(version?FVM_ERROR:FVM_OK));if(version)assert(c->avm1_steps==64);string_work_clean(c);rt->config.instruction_limit=0;}
  if(version){rt->config.instruction_limit=8;c->avm1_execution_depth=1;c->avm1_steps=7;
   assert(fvm_object_call(c,search,"indexOf",1,args,&out)==FVM_ERROR && c->avm1_steps==8 && c->avm1_execution_depth==1);c->avm1_execution_depth=0;
   rt->config.instruction_limit=0;c->avm1_execution_depth=1;c->avm1_steps=std::numeric_limits<unsigned long>::max();
   assert(fvm_object_call(c,search,"indexOf",1,args,&out)==FVM_ERROR && c->avm1_steps==std::numeric_limits<unsigned long>::max());c->avm1_execution_depth=0;
  }
  rt->config.instruction_limit=limit;assert(fvm_gc_collect(c,0)==FVM_OK);string_work_clean(c);fvm_context_destroy(c);
 }
 /* Copy accounting includes both UTF-16 units of a supplementary character. */
 FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);string_work_load(c,8);FVM_Object *str=string_work_make(c,"\xf0\x9f\x98\x80",4);FVM_Value out;
 unsigned long limit=rt->config.instruction_limit;rt->config.instruction_limit=2;assert(fvm_object_call(c,str,"split",0,0,&out)==FVM_ERROR);
 rt->config.instruction_limit=3;assert(fvm_object_call(c,str,"split",0,0,&out)==FVM_OK);number(c,out.object,"length",1);rt->config.instruction_limit=limit;
 string_work_clean(c);fvm_context_destroy(c);fvm_runtime_destroy(rt);
}
#endif
