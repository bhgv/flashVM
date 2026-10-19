/* Late parse/resource failures and subsequent retries preserve caller-owned outputs. */
#include "sound_metadata_fixture.hpp"
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    Bytes swf=metadata_movie();assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);
    /* Corrupt a canonical envelope's final gain after loading, keeping structure/size stable. */
    size_t corrupt=c->swf_bytes.size()-5;unsigned char saved=c->swf_bytes[corrupt];c->swf_bytes[corrupt]=255;
    for(unsigned route=3;route<6;++route)assert(metadata_query(c,route,true)==FVM_INVALID_ARGUMENT);
    c->swf_bytes[corrupt]=saved;
    for(unsigned route=0;route<6;++route)assert(metadata_query(c,route,false)==FVM_OK);
    FVM_SWFSoundInfo info;std::memset(&info,0xa5,sizeof(info));info.struct_size=sizeof(info);unsigned char old[sizeof(info)];std::memcpy(old,&info,sizeof(info));
    assert(fvm_swf_sound_at(c,1,&info)==FVM_NOT_FOUND && !std::memcmp(&info,old,sizeof(info)) && c->avm1_steps==6);
    c->gc_collecting=true;size_t count=12345;assert(fvm_swf_sound_event_count(c,&count)==FVM_INVALID_ARGUMENT && count==12345);c->gc_collecting=false;
    fvm_context_destroy(c);fvm_runtime_destroy(rt);
}
