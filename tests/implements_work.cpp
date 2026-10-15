/* Loaded interface snapshots charge capture and inspection before atomic publication. */
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/execution.h"
#include "flashvm/gc.h"
#include "../src/core/runtime.hpp"
#include <cassert>
#include <limits>
static FVM_Value declaration_class(FVM_Context *c,FVM_Object **prototype)
{
 const unsigned char code[]={0};FVM_Object *fn=0;assert(fvm_function_create_avm1(c,code,sizeof(code),0,0,0,&fn)==FVM_OK && fvm_gc_pin(c,fn)==FVM_OK);
 FVM_Value value,out;fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=fn;assert(fvm_object_get(c,fn,"prototype",&out)==FVM_OK);*prototype=out.object;return value;
}
static void declaration_load(FVM_Context *c,unsigned version)
{unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);}
static void declaration_clean(FVM_Context *c)
{assert(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);}
#ifndef FVM_IMPLEMENTS_WORK_FIXTURE_ONLY
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=4;version<=8;++version){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);declaration_load(c,version);
  FVM_Object *subproto=0,*ifaceproto=0;FVM_Value sub=declaration_class(c,&subproto),iface=declaration_class(c,&ifaceproto);
  fvm_detail::vector<FVM_Value> args;for(unsigned i=0;i<65536;++i)args.push_back(iface);
  unsigned long limit=rt->config.instruction_limit;rt->config.instruction_limit=8;c->avm1_steps=17;
  assert(fvm_exec_implements(c,&sub,(unsigned)args.size(),args.begin())==(version<7?FVM_OK:FVM_ERROR));
  assert(!subproto->interfaces_are_prototypes && subproto->implemented_interfaces.empty() && c->avm1_steps==(version<7?17UL:8UL));declaration_clean(c);
  rt->config.instruction_limit=0;assert(fvm_exec_implements(c,&sub,(unsigned)args.size(),args.begin())==FVM_OK);
  assert(subproto->interfaces_are_prototypes==(version>=7));
  if(version>=7){assert(subproto->implemented_interfaces.size()==65536 && c->avm1_steps==131073);for(size_t i=0;i<args.size();++i)assert(subproto->implemented_interfaces[i]==ifaceproto);
   FVM_Value primitive;fvm_value_number(&primitive,1);assert(fvm_exec_implements(c,&sub,1,&primitive)==FVM_OK && subproto->implemented_interfaces.size()==65536);
   rt->config.instruction_limit=1;assert(fvm_exec_implements(c,&sub,1,&iface)==FVM_ERROR && subproto->implemented_interfaces.size()==65536);
   c->avm1_execution_depth=1;c->avm1_steps=std::numeric_limits<unsigned long>::max();assert(fvm_exec_implements(c,&sub,1,&iface)==FVM_ERROR && c->avm1_execution_depth==1 && c->avm1_steps==std::numeric_limits<unsigned long>::max());c->avm1_execution_depth=0;
  }
  rt->config.instruction_limit=limit;assert(fvm_exec_implements(c,&sub,65537,args.begin())==FVM_INVALID_ARGUMENT);
  assert(fvm_gc_collect(c,0)==FVM_OK);declaration_clean(c);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);
}
#endif
