/* Legacy depth migration is bounded, precharged and shares an enclosing execution scope. */
#include "movieclip_tick_fixture.hpp"
#include "flashvm/clip_actions.h"
#include "../src/standard/movieclip_scene.hpp"
#include "../src/standard/movieclip_constants.h"
#include "../src/core/execution_work_scope.hpp"
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned nesting=0;nesting<2;++nesting)
    for(unsigned long limit=1;limit<=4;++limit)
    {
        rt->config.instruction_limit=0;FVM_Context *c=tick_context(rt);FVM_Object *root=tick_root(c);
        FVM_Object *a=tick_child(c,root,"a",1),*b=tick_child(c,root,"b",2),*d=tick_child(c,root,"d",3);
        FVM_Object *state=root->resource_result.object,*children=state->properties["children"].object;
        state->properties.erase("slot_bias");children->indexed.clear();children->indexed[1]=tick_object(a);children->indexed[2]=tick_object(b);children->indexed[3]=tick_object(d);
        rt->config.instruction_limit=limit;FVM_Object *out=root;
        if(nesting){c->avm1_execution_depth=1;c->avm1_steps=1;}
        const unsigned long required=3+nesting;
        assert(fvm_movieclip_scene_children(root,&out)==(limit<required?FVM_ERROR:FVM_OK));
        assert(c->avm1_execution_depth==nesting && c->avm1_steps==(limit<required?limit:required));
        c->avm1_execution_depth=0;tick_clean(c);
        if(limit<required){assert(out==root && children->indexed.find(1)!=children->indexed.end() && state->properties.find("slot_bias")==state->properties.end());}
        else {assert(out==children && children->indexed.find(1)==children->indexed.end() && children->indexed.find(1+FVM_MOVIECLIP_CHILD_SLOT_BIAS)->second.object==a);}
        if(limit<required){rt->config.instruction_limit=0;assert(fvm_movieclip_scene_children(root,&out)==FVM_OK && out==children);}
        {FVM_ExecutionWorkScope work(c);assert(work.step()==FVM_OK);unsigned long before=c->avm1_steps;assert(fvm_movieclip_scene_children(root,&out)==FVM_OK && c->avm1_steps==before);}
        fvm_context_destroy(c);
    }
    rt->config.instruction_limit=0;FVM_Context *c=tick_context(rt);FVM_Object *root=tick_root(c);
    FVM_Object *a=tick_child(c,root,"a",1),*state=root->resource_result.object,*children=state->properties["children"].object;
    state->properties.erase("slot_bias");children->indexed.clear();children->indexed[1]=tick_object(a);
    rt->config.instruction_limit=1;assert(fvm_movieclip_dispatch_clip_input(c,root,FVM_CLIP_EVENT_MOUSE_MOVE)==FVM_ERROR);
    assert(state->properties.find("slot_bias")==state->properties.end() && children->indexed.find(1)!=children->indexed.end());tick_clean(c);
    rt->config.instruction_limit=0;assert(fvm_movieclip_dispatch_clip_input(c,root,FVM_CLIP_EVENT_MOUSE_MOVE)==FVM_OK);tick_clean(c);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);
}
