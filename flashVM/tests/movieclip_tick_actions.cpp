/* Separate frame scripts must not reset the outer tick budget in interpreter or native JIT. */
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#include "movieclip_tick_fixture.hpp"
int main()
{
 for(unsigned mode=0;mode<2;++mode){if(mode && !fvm_jit_available())continue;FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK);cfg.instruction_limit=11;assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);FVM_Context *c=tick_context(rt);FVM_Object *root=tick_root(c,2);TickProbe probe;
  Program p;p.text("seen");p.number(42);p.op(0x1d);p.op(0);for(unsigned frame=1;frame<=2;++frame)assert(fvm_movieclip_define_frame(c,root,frame,0,p.bytes.begin(),p.bytes.size())==FVM_OK);
  assert(fvm_object_define_method_rooted(c,root,"onEnterFrame",tick_notify,&probe,0)==FVM_OK && fvm_execution_set_mode(c,mode?FVM_EXECUTION_JIT:FVM_EXECUTION_INTERPRETER,0)==FVM_OK);
  assert(fvm_movieclip_advance(c,root,3)==FVM_ERROR && probe.calls==1 && c->avm1_steps==11);FVM_Value value;assert(fvm_object_get(c,root,"seen",&value)==FVM_OK && value.number==42);assert(fvm_object_get(c,root,"_currentframe",&value)==FVM_OK && value.number==2);tick_clean(c);if(mode)assert(c->execution_mode.stats.native_entries>0);fvm_context_destroy(c);fvm_runtime_destroy(rt);
 }
}
