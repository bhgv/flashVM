/* Actual retained event bytecode executes in interpreter and automatic native JIT with equal state. */
#include "clip_actions_fixture.hpp"
#include "flashvm/execution.h"
#include "flashvm/jit.h"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=5;version<=8;++version)for(unsigned mode=0;mode<2;++mode){if(mode && !fvm_jit_available())continue;FVM_Context *c=clip_context(rt);assert(fvm_execution_set_mode(c,mode?FVM_EXECUTION_JIT:FVM_EXECUTION_INTERPRETER,0)==FVM_OK);FVM_Object *root=clip_load(c,clip_movie(version,clip_event_table(version,3,action(42))));assert(fvm_movieclip_advance(c,root,1)==FVM_OK);FVM_Object *actor=get(c,root,"actor").object;number(c,actor,"seen",42);
  unsigned long before=c->execution_mode.stats.native_entries;assert(fvm_movieclip_dispatch_clip_actions(c,actor,1,0)==FVM_OK && c->execution_mode.stats.native_entries==before+mode);number(c,actor,"seen",42);clip_clean(c);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);
}
