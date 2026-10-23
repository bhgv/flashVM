/* A real finite-event worker holds all newly started voices until owner frame publication. */
#include "../common/audio.h"
#include "../../tests/swf_sound_conversion_fixture.hpp"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
using namespace conversion_fixture;
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK&&fvm_runtime_create(&cfg,&rt)==FVM_OK&&fvm_context_create(rt,&c)==FVM_OK&&fvm_standard_install(c)==FVM_OK);
 Samples samples;for(unsigned i=0;i<8;++i)samples.push_back(2000);Bytes tags;definition(tags,7,3,1,samples);event(tags,7);event(tags,7);Bytes swf=sound_fixture::movie(tags);FVM_Object *clip=0;
 assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK&&fvm_movieclip_load_swf(c,swf.begin(),swf.size(),"root",&clip)==FVM_OK&&fvm_gc_pin(c,clip)==FVM_OK);
 FVM_DesktopAudio audio;assert(fvm_desktop_audio_open_policy(c,clip,&audio,0,0,1)==FVM_OK);SDL_Delay(30);FVM_SWFSoundOutputInfo info;info.struct_size=sizeof(info);assert(fvm_desktop_audio_info(&audio,&info)==FVM_OK&&!info.submitted_frames);
 assert(fvm_movieclip_advance(c,clip,1)==FVM_OK&&audio.events==2);SDL_Delay(30);assert(fvm_desktop_audio_info(&audio,&info)==FVM_OK&&!info.submitted_frames);
 assert(fvm_gc_collect(c,0)==FVM_OK&&fvm_desktop_audio_timeline(c,clip,&audio)==FVM_OK);
 assert(SDL_LockMutex(audio.mutex)==0);assert(audio.event_frame_ready);SDL_UnlockMutex(audio.mutex);
 int pending=1;for(unsigned i=0;i<50&&pending;++i){assert(fvm_desktop_audio_stream_pending(&audio,&pending)==FVM_OK);if(pending)SDL_Delay(10);}
 assert(!pending&&fvm_desktop_audio_info(&audio,&info)==FVM_OK&&info.submitted_frames==9&&info.played_frames==9);
 assert(SDL_AtomicGet(&audio.underruns)==0);
 assert(fvm_movieclip_set_sound_callback(c,0,0)==FVM_OK);fvm_desktop_audio_close(&audio);fvm_gc_unpin(c,clip);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
