/* Allocation failures unwind method/owner/work scopes while committed timeline state remains usable. */
#define main tick_allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#include "movieclip_tick_fixture.hpp"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);bool success=false;unsigned failures=0;
 for(long boundary=0;boundary<512 && !success;++boundary){FVM_Context *c=tick_context(rt);FVM_Object *root=tick_root(c,2);TickProbe p;
  for(unsigned i=0;i<3;++i){FVM_Object *ch=tick_child(c,root,"child",i);assert(fvm_object_define_method_rooted(c,ch,"onEnterFrame",tick_notify,&p,0)==FVM_OK);}
  assert(fvm_object_define_method_rooted(c,root,"onEnterFrame",tick_notify,&p,0)==FVM_OK);allowance=boundary;FVM_Result r=fvm_movieclip_advance(c,root,1);allowance=-1;tick_clean(c);
  if(r==FVM_OK){success=true;assert(p.calls==4);}else{assert(r==FVM_OUT_OF_MEMORY);++failures;unsigned before=p.calls;assert(fvm_movieclip_advance(c,root,1)==FVM_OK && p.calls==before+4);tick_clean(c);}
  assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
 }
 assert(success && failures);fvm_runtime_destroy(rt);
}
