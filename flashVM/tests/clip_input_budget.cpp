/* Empty trees charge capture/delivery and nested selected calls; a batch never resets native work. */
#include "clip_input_fixture.hpp"
int main()
{
 for(unsigned limit=1;limit<=3;++limit){FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK);cfg.instruction_limit=limit;assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);FVM_Context *c=clip_context(rt);FVM_Object *root=0;assert(fvm_movieclip_create(c,"root",1,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK);assert(fvm_movieclip_dispatch_clip_input(c,root,8)==(limit<3?FVM_ERROR:FVM_OK) && c->avm1_steps==limit);input_clean(c);fvm_context_destroy(c);fvm_runtime_destroy(rt);}
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);FVM_Context *c=clip_context(rt);FVM_Object *root=0,*actor=input_actor(c,&root),*b=input_duplicate(c,actor,"b",3);rt->config.instruction_limit=50;assert(fvm_movieclip_dispatch_clip_input(c,root,8)==FVM_ERROR);number(c,actor,"seen",42);assert(get(c,b,"seen").type==FVM_VALUE_UNDEFINED);input_clean(c);rt->config.instruction_limit=0;assert(fvm_movieclip_dispatch_clip_input(c,root,8)==FVM_OK);number(c,b,"seen",42);fvm_context_destroy(c);fvm_runtime_destroy(rt);
}
