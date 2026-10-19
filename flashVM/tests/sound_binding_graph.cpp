/* Frame/source identity survives queue/file transport and source destruction; callbacks remain local. */
#include "sound_snapshot_fixture.hpp"
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=6;version<=8;++version)for(unsigned route=0;route<2;++route)for(unsigned mode=0;mode<2;++mode)
    {
        if(mode && !fvm_jit_available())continue;
        FVM_Context *source=tick_context(rt,version),*c=tick_context(rt,version);SoundSnapshot from,to;FVM_Object *root=sound_snapshot_root(source,from,version);
        Bytes swf=sound_movie();swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);
        FVM_Value input=tick_object(root),received;
        if(!route){unsigned char *bytes=0;size_t size=0;assert(fvm_value_serialize(source,&input,&bytes,&size)==FVM_OK && fvm_value_deserialize(c,bytes,size,&received)==FVM_OK);fvm_serialized_free(bytes);}
        else{FVM_Channel *ch=0;assert(fvm_channel_create(&ch)==FVM_OK && fvm_channel_send_value(source,ch,&input,0)==FVM_OK && fvm_channel_receive_value(c,ch,&received,0)==FVM_OK);fvm_channel_destroy(ch);}
        root=received.object;assert(fvm_gc_pin(c,root)==FVM_OK);fvm_context_destroy(source);
        FVM_Object *frames=root->resource_result.object->properties["frames"].object;to.list=frames->indexed[1].object->properties["sounds"].object;
        assert(fvm_gc_string_mode(c,FVM_GC_STRINGS_TRACED)==FVM_OK && fvm_gc_collect(c,0)==FVM_OK);
        assert(fvm_movieclip_set_sound_callback(c,sound_snapshot_receive,&to)==FVM_OK);
        assert(fvm_execution_set_mode(c,mode?FVM_EXECUTION_JIT:FVM_EXECUTION_INTERPRETER,0)==FVM_OK);
        FVM_Value saved=to.list->indexed[1];to.list->indexed[1]=to.list->indexed[0];seek(c,root,1,false);
        assert(fvm_movieclip_advance(c,root,1)==FVM_INVALID_ARGUMENT && !to.count);tick_clean(c);
        to.list->indexed[1]=saved;seek(c,root,1,false);assert(fvm_movieclip_advance(c,root,1)==FVM_OK && to.count==2 && to.delivered[0]==0 && to.delivered[1]==1);
        if(mode)assert(c->execution_mode.stats.native_entries>0);tick_clean(c);assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
