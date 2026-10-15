/* Loaded InstanceOf visits large cyclic interface graphs within the shared work budget. */
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/execution.h"
#include "flashvm/gc.h"
#include "../src/core/runtime.hpp"
#include <cassert>
#include <limits>
static FVM_Value graph_value(FVM_Object *o)
{FVM_Value v;fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=o;return v;}
static FVM_Value graph_setup(FVM_Context *c,unsigned count,FVM_Value *constructor,bool connected)
{
 const unsigned char code[]={0};FVM_Object *fn=0,*base=0,*instance=0;
 assert(fvm_function_create_avm1(c,code,sizeof(code),0,0,0,&fn)==FVM_OK && fvm_gc_pin(c,fn)==FVM_OK);
 *constructor=graph_value(fn);FVM_Value proto;assert(fvm_object_get(c,fn,"prototype",&proto)==FVM_OK);
 assert(fvm_object_create(c,&base)==FVM_OK && fvm_object_create(c,&instance)==FVM_OK);
 assert(fvm_object_set_prototype(c,instance,base)==FVM_OK && fvm_gc_pin(c,instance)==FVM_OK);
 base->interfaces_are_prototypes=true;
 if(connected)base->implemented_interfaces.push_back(proto.object);
 for(unsigned i=0;i<count;++i){FVM_Object *edge=0;assert(fvm_object_create(c,&edge)==FVM_OK);edge->interfaces_are_prototypes=true;edge->implemented_interfaces.push_back(base);base->implemented_interfaces.push_back(edge);}
 return graph_value(instance);
}
static void graph_load(FVM_Context *c,unsigned version)
{unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);}
#ifndef FVM_INSTANCE_GRAPH_FIXTURE_ONLY
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=4;version<=8;++version)for(unsigned connected=0;connected<2;++connected) {
  FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);graph_load(c,version);
  FVM_Value klass,out,input=graph_setup(c,8192,&klass,connected!=0);
  assert(fvm_exec_instance_of(c,&input,&klass,&out)==FVM_OK && out.type==FVM_VALUE_BOOLEAN && out.boolean==(int)connected);
  assert(c->avm1_steps>8192 && !c->gc_frames && !c->avm1_execution_depth && !c->gc_native_depth);
  assert(fvm_gc_collect(c,0)==FVM_OK);unsigned long limit=rt->config.instruction_limit;rt->config.instruction_limit=8;fvm_value_number(&out,77);
  assert(fvm_exec_instance_of(c,&input,&klass,&out)==FVM_ERROR && out.type==FVM_VALUE_NUMBER && out.number==77 && c->avm1_steps==8 && !c->avm1_execution_depth);
  c->avm1_execution_depth=1;c->avm1_steps=7;
  assert(fvm_exec_instance_of(c,&input,&klass,&out)==FVM_ERROR && out.number==77 && c->avm1_steps==8 && c->avm1_execution_depth==1);c->avm1_execution_depth=0;
  c->avm1_execution_depth=1;c->avm1_steps=std::numeric_limits<unsigned long>::max();
  assert(fvm_exec_instance_of(c,&input,&klass,&out)==FVM_ERROR && out.number==77 && c->avm1_steps==std::numeric_limits<unsigned long>::max() && c->avm1_execution_depth==1);c->avm1_execution_depth=0;
  rt->config.instruction_limit=0;FVM_Value alias=input;
  assert(fvm_exec_instance_of(c,&alias,&klass,&alias)==FVM_OK && alias.type==FVM_VALUE_BOOLEAN && alias.boolean==(int)connected);
  rt->config.instruction_limit=limit;assert(!c->gc_frames && !c->gc_native_depth);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);
}
#endif
