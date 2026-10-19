/* Imported retained input trees preserve source policy and interpreter/native state after sender teardown. */
#include "clip_input_fixture.hpp"
#include "flashvm/execution_mode.h"
#include "flashvm/jit.h"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned mode=0;mode<2;++mode){if(mode && !fvm_jit_available())continue;FVM_Context *c=clip_context(rt),*dest=clip_context(rt);FVM_Object *root=0,*actor=input_actor(c,&root);input_duplicate(c,actor,"b",3);FVM_Value value;fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=root;unsigned char *wire=0;size_t size=0;assert(fvm_value_serialize(c,&value,&wire,&size)==FVM_OK);fvm_context_destroy(c);assert(fvm_value_deserialize(dest,wire,size,&value)==FVM_OK);fvm_serialized_free(wire);root=value.object;assert(fvm_gc_pin(dest,root)==FVM_OK && fvm_gc_string_mode(dest,FVM_GC_STRINGS_TRACED)==FVM_OK && fvm_gc_collect(dest,0)==FVM_OK && !dest->clip_input_dispatching);
  assert(fvm_execution_set_mode(dest,mode?FVM_EXECUTION_JIT:FVM_EXECUTION_INTERPRETER,0)==FVM_OK);unsigned long before=dest->execution_mode.stats.native_entries;assert(fvm_movieclip_dispatch_clip_input(dest,root,8)==FVM_OK && dest->execution_mode.stats.native_entries==before+(mode?2:0));number(dest,get(dest,root,"actor").object,"seen",42);number(dest,get(dest,root,"b").object,"seen",42);input_clean(dest);fvm_context_destroy(dest);
 }
 fvm_runtime_destroy(rt);
}
