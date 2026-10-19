/* Local child snapshots reject malformed values/ownership/aliases before any child callbacks. */
#include "movieclip_tick_order_fixture.hpp"
/* Root delivery commits before validating its post-handler child table. */
int main()
{
    FVM_Config cfg;FVM_Runtime *runtime=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&runtime)==FVM_OK);
    for(unsigned mode=0;mode<5;++mode)
    {
        FVM_Context *c=tick_context(runtime),*other=tick_context(runtime);
        FVM_Object *root=tick_root(c),*child=ordered_child(c,root,"child",1),*foreign=tick_root(other);
        FVM_Object *children=0;assert(fvm_movieclip_storage::private_object(root->resource_result.object,"children",children)==FVM_OK);
        TickProbe parent_probe,child_probe;
        assert(fvm_object_define_method_rooted(c,root,"onEnterFrame",tick_notify,&parent_probe,0)==FVM_OK);
        assert(fvm_object_define_method_rooted(c,child,"onEnterFrame",tick_notify,&child_probe,0)==FVM_OK);
        FVM_Value invalid;
        if(mode==0)fvm_value_number(&invalid,17);
        if(mode==1)invalid=tick_object(foreign);
        if(mode==2)invalid=tick_object(child);
        if(mode==3)invalid=tick_object(root);
        FVM_Object *plain=0;
        if(mode==4){assert(fvm_object_create(c,&plain)==FVM_OK);invalid=tick_object(plain);}
        children->indexed[999]=invalid;
        assert(fvm_movieclip_advance(c,root,1)==FVM_INVALID_ARGUMENT && parent_probe.calls==1 && !child_probe.calls);
        children->indexed.erase(999);assert(fvm_movieclip_advance(c,root,1)==FVM_OK && child_probe.calls==1);
        tick_clean(c);fvm_context_destroy(c);fvm_context_destroy(other);
    }
    fvm_runtime_destroy(runtime);
}
