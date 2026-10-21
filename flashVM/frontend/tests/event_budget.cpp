/* Actual owner callback applies aggregate event storage and preserves native producer access. */
#include "../common/audio.h"
#include "../../tests/swf_sound_conversion_fixture.hpp"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
using namespace conversion_fixture;
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);
    Samples small,large;small.resize(12000);large.resize(40000);
    for(size_t i=0;i<small.size();++i)small[i]=1234;
    Bytes tags;definition(tags,7,3,1,small);definition(tags,8,3,1,large);
    event(tags,7);event(tags,7,FVM_SWF_SOUND_EVENT_NO_MULTIPLE);event(tags,8);
    Bytes swf=sound_fixture::movie(tags);FVM_Object *clip=0;
    assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK && fvm_movieclip_load_swf(c,swf.begin(),swf.size(),"root",&clip)==FVM_OK && fvm_gc_pin(c,clip)==FVM_OK);
    FVM_DesktopAudio audio;assert(fvm_desktop_audio_open_paused(c,clip,&audio,1)==FVM_OK);
    /* One owner frame runs all three real callbacks: the oversized third owner fails. */
    assert(fvm_movieclip_advance(c,clip,1)==FVM_OUT_OF_MEMORY && audio.events==2);
    assert(SDL_LockMutex(audio.mutex)==0);
    FVM_SWFSoundMixerStorageInfo storage;storage.struct_size=sizeof(storage);
    assert(fvm_swf_sound_mixer_storage_info(audio.mixer,&storage)==FVM_OK && storage.retained_bytes<=FVM_DESKTOP_AUDIO_EVENT_BYTES);
    FVM_SWFSoundMixerInfo info;info.struct_size=sizeof(info);
    assert(fvm_swf_sound_mixer_info(audio.mixer,&info)==FVM_OK && info.active_voices==1);
    SDL_UnlockMutex(audio.mutex);
    assert(fvm_desktop_audio_pause(&audio,0)==FVM_OK);SDL_Delay(80);
    FVM_SWFSoundOutputInfo output;output.struct_size=sizeof(output);
    assert(fvm_desktop_audio_info(&audio,&output)==FVM_OK && output.submitted_frames>0 && SDL_AtomicGet(&audio.error)==FVM_OK);
    assert(fvm_gc_collect(c,0)==FVM_OK && fvm_audio_stop_all(c)==FVM_OK);
    assert(SDL_LockMutex(audio.mutex)==0);
    assert(fvm_swf_sound_mixer_storage_info(audio.mixer,&storage)==FVM_OK && !storage.voice_bytes);
    SDL_UnlockMutex(audio.mutex);
    assert(fvm_movieclip_set_sound_callback(c,0,0)==FVM_OK);fvm_desktop_audio_close(&audio);
    fvm_gc_unpin(c,clip);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
