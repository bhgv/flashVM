/* Real SDL worker verifies finite event host pause, native JIT/GC, expected drain and anonymous release. */
#include "../common/audio.h"
#include "../../tests/swf_sound_conversion_fixture.hpp"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/execution_mode.h"
using namespace conversion_fixture;
static void drain(FVM_DesktopAudio *audio)
{
 int pending=1;for(unsigned i=0;i<100&&pending;++i){assert(fvm_desktop_audio_stream_pending(audio,&pending)==FVM_OK);if(pending)SDL_Delay(5);}assert(!pending);
}
int main()
{
 for(unsigned mode=0;mode<2;++mode)
 {
  if(mode&&!fvm_jit_available())continue;
  FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK&&fvm_runtime_create(&cfg,&rt)==FVM_OK&&fvm_context_create(rt,&c)==FVM_OK&&fvm_standard_install(c)==FVM_OK&&fvm_execution_set_mode(c,mode?FVM_EXECUTION_JIT:FVM_EXECUTION_INTERPRETER,0)==FVM_OK);
  Samples positive,negative;for(unsigned i=0;i<8;++i)positive.push_back(30000);for(unsigned i=0;i<12;++i)negative.push_back(-30000);Bytes tags,action;definition(tags,7,3,1,positive);definition(tags,8,3,1,negative);event(tags,7);event(tags,8);action.push_back(7);action.push_back(0);sound_fixture::tag(tags,12,action);Bytes swf=sound_fixture::movie(tags);FVM_Object *clip=0;
  assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK&&fvm_movieclip_load_swf(c,swf.begin(),swf.size(),"root",&clip)==FVM_OK&&fvm_gc_pin(c,clip)==FVM_OK);
  FVM_DesktopAudio audio;assert(fvm_desktop_audio_open_policy(c,clip,&audio,1,0,1)==FVM_OK);
  assert(fvm_movieclip_advance(c,clip,1)==FVM_OK&&fvm_desktop_audio_timeline(c,clip,&audio)==FVM_OK&&audio.events==2);
  SDL_Delay(30);FVM_SWFSoundOutputInfo info;info.struct_size=sizeof(info);assert(fvm_desktop_audio_info(&audio,&info)==FVM_OK&&!info.submitted_frames&&info.paused);
  assert(fvm_gc_collect(c,0)==FVM_OK&&fvm_desktop_audio_pause(&audio,0)==FVM_OK);drain(&audio);
  assert(fvm_desktop_audio_info(&audio,&info)==FVM_OK&&info.submitted_frames==14&&info.played_frames==14&&!info.pending_frames);
  assert(fvm_audio_stop_all(c)==FVM_OK&&audio.stops==1);drain(&audio);
  assert(fvm_desktop_audio_info(&audio,&info)==FVM_OK&&info.submitted_frames==240&&info.played_frames==info.submitted_frames);
  assert(SDL_AtomicGet(&audio.error)==FVM_OK&&SDL_AtomicGet(&audio.underruns)==0);FVM_ExecutionStats stats;stats.struct_size=sizeof(stats);assert(fvm_execution_get_stats(c,&stats)==FVM_OK&&(mode?stats.native_entries:stats.interpreter_entries)>0);
  assert(fvm_movieclip_set_sound_callback(c,0,0)==FVM_OK);fvm_desktop_audio_close(&audio);fvm_gc_unpin(c,clip);fvm_context_destroy(c);fvm_runtime_destroy(rt);
 }
 return 0;
}
