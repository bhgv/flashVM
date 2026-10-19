/* Capture OOM has no script effects; all capture/record/interpreter allocations unwind the guard. */
#define main input_allocation_fixture_main
#include "swf_sprite_allocation.cpp"
#undef main
#include "clip_input_fixture.hpp"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);bool complete=false;unsigned failures=0;
 for(long n=0;n<2048 && !complete;++n){FVM_Context *c=clip_context(rt);FVM_Object *root=0,*actor=input_actor(c,&root);input_duplicate(c,actor,"b",3);allowance=n;FVM_Result r=fvm_movieclip_dispatch_clip_input(c,root,8);allowance=-1;assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY);input_clean(c);if(r==FVM_OK)complete=true;else{++failures;assert(fvm_movieclip_dispatch_clip_input(c,root,8)==FVM_OK);}number(c,actor,"seen",42);assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);}assert(complete && failures);fvm_runtime_destroy(rt);
}
