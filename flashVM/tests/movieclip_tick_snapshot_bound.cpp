/* Oversized post-handler tables reject before charging/copying every slot or calling children. */
#include "movieclip_tick_order_fixture.hpp"
/* Use a bounded malformed host fixture, not tens of thousands of live native clips. */
int main()
{
    FVM_Config cfg;FVM_Runtime *runtime=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&runtime)==FVM_OK);
    FVM_Context *c=tick_context(runtime);FVM_Object *root=tick_root(c),*child=ordered_child(c,root,"child",1),*children=0;
    assert(fvm_movieclip_storage::private_object(root->resource_result.object,"children",children)==FVM_OK);
    FVM_Value value=tick_object(child);
    for(unsigned long slot=0;slot<65537UL;++slot)children->indexed[slot]=value;
    TickProbe root_probe,child_probe;
    assert(fvm_object_define_method_rooted(c,root,"onEnterFrame",tick_notify,&root_probe,0)==FVM_OK);
    assert(fvm_object_define_method_rooted(c,child,"onEnterFrame",tick_notify,&child_probe,0)==FVM_OK);
    assert(fvm_movieclip_advance(c,root,1)==FVM_INVALID_ARGUMENT);
    assert(root_probe.calls==1 && !child_probe.calls && c->avm1_steps==2);
    tick_clean(c);fvm_context_destroy(c);fvm_runtime_destroy(runtime);
}
