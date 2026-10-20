/* Explicit stream omission is exact-source-bound and never relaxes strict imports. */
#include "flashvm/movieclip.h"
#include "flashvm/swf_stream_sound.h"
#include "flashvm/gc.h"
#include "flashvm/standard.h"
#include "swf_sound_fixture.hpp"
using namespace sound_fixture;
static Bytes source(bool malformed, bool sprite)
{
    Bytes tags,h,pcm,empty;
    h.push_back(12);h.push_back(0x3e);word(h,1);
    word(pcm,4096);if(malformed)pcm.push_back(1);
    tag(tags,45,h);tag(tags,19,pcm);
    if(sprite)
    {
        Bytes nested,controls;word(nested,7);word(nested,1);
        tag(controls,45,h);tag(controls,19,pcm);tag(controls,1,empty);tag(controls,0,empty);
        for(size_t i=0;i<controls.size();++i)
            nested.push_back(controls[i]);
        tag(tags,39,nested);
    }
    return movie(tags,8);
}
int main()
{
    FVM_Config cfg;FVM_Runtime *r=0;FVM_Context *c=0;FVM_Object *clip=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&r)==FVM_OK);
    assert(fvm_context_create(r,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);
    Bytes good=source(false,false),bad=source(true,false),nested=source(false,true);
    assert(fvm_movieclip_load_swf_ex(c,good.begin(),good.size(),"root",FVM_SWF_IMPORT_MUTE_ROOT_STREAMS,&clip)==FVM_INVALID_ARGUMENT && !clip);
    assert(fvm_load_memory(c,good.begin(),good.size())==FVM_OK);
    assert(fvm_movieclip_load_swf(c,good.begin(),good.size(),"root",&clip)==FVM_UNSUPPORTED && !clip);
    assert(fvm_movieclip_load_swf_ex(c,good.begin(),good.size(),"root",2,&clip)==FVM_INVALID_ARGUMENT && !clip);
    assert(fvm_movieclip_load_swf_ex(c,bad.begin(),bad.size(),"root",FVM_SWF_IMPORT_MUTE_ROOT_STREAMS,&clip)==FVM_INVALID_ARGUMENT && !clip);
    assert(fvm_movieclip_load_swf_ex(c,good.begin(),good.size(),"root",FVM_SWF_IMPORT_MUTE_ROOT_STREAMS,&clip)==FVM_OK && clip);
    assert(fvm_gc_pin(c,clip)==FVM_OK && fvm_movieclip_advance(c,clip,1)==FVM_OK);
    assert(fvm_gc_step(c,0)==FVM_OK && fvm_movieclip_advance(c,clip,1)==FVM_OK);
    assert(fvm_gc_unpin(c,clip)==FVM_OK);
    assert(fvm_load_memory(c,bad.begin(),bad.size())==FVM_OK);
    assert(fvm_movieclip_load_swf_ex(c,bad.begin(),bad.size(),"bad",FVM_SWF_IMPORT_MUTE_ROOT_STREAMS,&clip)==FVM_INVALID_ARGUMENT && !clip);
    assert(fvm_load_memory(c,nested.begin(),nested.size())==FVM_OK);
    assert(fvm_movieclip_load_swf_ex(c,nested.begin(),nested.size(),"nested",FVM_SWF_IMPORT_MUTE_ROOT_STREAMS,&clip)==FVM_UNSUPPORTED && !clip);
    fvm_context_destroy(c);fvm_runtime_destroy(r);return 0;
}
