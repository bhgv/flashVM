/* Real SDL producer must keep native PCM moving while the VM/render thread deliberately stalls. */
#include "../common/audio.h"
#include "../../tests/swf_sound_conversion_fixture.hpp"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include <cassert>
using namespace conversion_fixture;
int main()
{
    FVM_Config cfg;FVM_Runtime *runtime=0;FVM_Context *context=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&runtime)==FVM_OK);
    assert(fvm_context_create(runtime,&context)==FVM_OK && fvm_standard_install(context)==FVM_OK);
    Samples samples;for(unsigned i=0;i<44100;++i)samples.push_back((short)((i%64)*256-8192));
    Bytes tags;definition(tags,7,3,1,samples);event(tags,7,0);
    Bytes swf=sound_fixture::movie(tags);
    assert(fvm_load_memory(context,swf.begin(),swf.size())==FVM_OK);
    FVM_Object *clip=0;assert(fvm_movieclip_load_swf(context,swf.begin(),swf.size(),"root",&clip)==FVM_OK);
    assert(fvm_gc_pin(context,clip)==FVM_OK);
    FVM_DesktopAudio audio;
    assert(fvm_desktop_audio_open(context,clip,&audio)==FVM_OK && audio.thread);
#ifdef FVM_TEST_NATIVE_QUANTUM
    assert(audio.target_frames >= FVM_TEST_NATIVE_QUANTUM * 4U);
#endif
    assert(audio.target_frames>=4800 && audio.resume_frames<=audio.target_frames);
    assert(fvm_movieclip_advance(context,clip,1)==FVM_OK && audio.events==1);
    SDL_Delay(350); /* No tick/render-thread refill: the old 43 ms design starved here. */
    FVM_SWFSoundOutputInfo before,after;before.struct_size=sizeof(before);after.struct_size=sizeof(after);
    assert(fvm_desktop_audio_info(&audio,&before)==FVM_OK && before.played_frames>10000);
    assert(before.queued_frames>0 && SDL_AtomicGet(&audio.error)==FVM_OK);
    assert(fvm_desktop_audio_pause(&audio,1)==FVM_OK);
    assert(fvm_desktop_audio_info(&audio,&before)==FVM_OK && before.paused);
    SDL_Delay(60);
    assert(fvm_desktop_audio_info(&audio,&after)==FVM_OK && after.played_frames==before.played_frames);
    assert(fvm_desktop_audio_tick(&audio)==FVM_OK);
    SDL_Delay(100);
    assert(fvm_desktop_audio_info(&audio,&after)==FVM_OK && after.played_frames>before.played_frames);
    assert(fvm_audio_stop_all(context)==FVM_OK && audio.stops==1);
    assert(SDL_LockMutex(audio.mutex)==0);
    FVM_SWFSoundMixerInfo mixer_info;mixer_info.struct_size=sizeof(mixer_info);
    assert(fvm_swf_sound_mixer_info(audio.mixer,&mixer_info)==FVM_OK && !mixer_info.active_voices);
    SDL_UnlockMutex(audio.mutex);
    assert(fvm_movieclip_set_sound_callback(context,0,0)==FVM_OK);
    fvm_desktop_audio_close(&audio);
    assert(!audio.thread && !audio.mutex && !audio.device);
    fvm_gc_unpin(context,clip);fvm_context_destroy(context);fvm_runtime_destroy(runtime);
    return 0;
}
