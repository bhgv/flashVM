/* Serialized and queued scenes retain snapshot/work behavior after destroying their source. */
#include "scene_controls_fixture.hpp"
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=6;version<=8;++version)for(unsigned route=0;route<2;++route)for(unsigned mode=0;mode<2;++mode)
    {
        if(mode && !fvm_jit_available())continue;
        FVM_Context *source=tick_context(rt,version),*c=tick_context(rt,version);SceneControlProbe from,to;
        FVM_Object *root=scene_control_root(source,from,version,false);FVM_Value value=tick_object(root),received;
        if(!route){unsigned char *bytes=0;size_t size=0;assert(fvm_value_serialize(source,&value,&bytes,&size)==FVM_OK);
            assert(fvm_value_deserialize(c,bytes,size,&received)==FVM_OK);fvm_serialized_free(bytes);}
        else{FVM_Channel *ch=0;assert(fvm_channel_create(&ch)==FVM_OK && fvm_channel_send_value(source,ch,&value,0)==FVM_OK);
            assert(fvm_channel_receive_value(c,ch,&received,0)==FVM_OK);fvm_channel_destroy(ch);}
        root=received.object;assert(fvm_gc_pin(c,root)==FVM_OK);fvm_context_destroy(source);
        /* Register a destination-owned constructor; borrowed userdata never crosses the graph. */
        (void)scene_control_root(c,to,version);
        to.list=root->resource_result.object->properties["frames"].object->indexed[1].object->properties["controls"].object;
        to.future=to.list->indexed[1].object;to.mutation=1;to.calls=0;
        assert(fvm_gc_string_mode(c,FVM_GC_STRINGS_TRACED)==FVM_OK && fvm_gc_collect(c,0)==FVM_OK);
        assert(fvm_execution_set_mode(c,mode?FVM_EXECUTION_JIT:FVM_EXECUTION_INTERPRETER,0)==FVM_OK);
        rt->config.instruction_limit=3;assert(fvm_movieclip_advance(c,root,1)==FVM_ERROR && !to.calls);tick_clean(c);
        rt->config.instruction_limit=0;assert(fvm_movieclip_advance(c,root,1)==FVM_OK && to.calls==3);tick_clean(c);
        assert(to.depths[0]==-16383 && to.depths[1]==-16382 && to.depths[2]==-16381);
        if(mode)assert(c->execution_mode.stats.native_entries>0);
        assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
