/* Canonical indexing is one charged scan with transactional outputs, malformed tails and a hard cap. */
#include "sound_metadata_fixture.hpp"
#include "../src/swf/sound_event_index.hpp"
#include "../src/swf/sound_event_constants.h"
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    Bytes swf=metadata_movie();assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);
    for(unsigned long limit=0;limit<=12;++limit)
    {
        fvm_detail::vector<size_t> out;out.push_back(999);rt->config.instruction_limit=limit;
        assert(fvm_swf_sound_frame_ordinals(c,1,out)==(limit && limit<11?FVM_ERROR:FVM_OK));
        assert(c->avm1_steps==(limit && limit<11?limit:11));
        if(limit && limit<11)assert(out.size()==1 && out[0]==999);else assert(out.size()==2 && out[0]==0 && out[1]==1);
    }
    rt->config.instruction_limit=0;fvm_detail::vector<size_t> out;out.push_back(999);
    assert(fvm_swf_sound_frame_ordinals(c,0,out)==FVM_INVALID_ARGUMENT && out.size()==1 && out[0]==999);
    assert(fvm_swf_sound_frame_ordinals(c,2,out)==FVM_INVALID_ARGUMENT && out.size()==1 && out[0]==999);
    c->swf_bytes[c->swf_bytes.size()-5]=255;
    assert(fvm_swf_sound_frame_ordinals(c,1,out)==FVM_INVALID_ARGUMENT && out.size()==1 && out[0]==999);
    Bytes tags,body;word(body,7);body.push_back(0);
    for(unsigned long i=0;i<=FVM_SWF_SOUND_MAX_FRAME_ORDINALS;++i)tag(tags,15,body);
    swf=movie(tags);assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);
    assert(fvm_swf_sound_frame_ordinals(c,1,out)==FVM_INVALID_ARGUMENT && out.size()==1 && out[0]==999);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);
}
