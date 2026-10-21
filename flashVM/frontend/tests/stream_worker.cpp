/* Real SDL dummy queue receives only native PCM authorized by actual MovieClip entry. */
#include "../common/audio.h"
#include "../../tests/swf_sound_fixture.hpp"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/execution_mode.h"
#include <cassert>
using namespace sound_fixture;
static void method(FVM_Context *c,FVM_Object *o,const char *name,unsigned frame=0)
{ FVM_Value a,v;fvm_value_number(&a,frame);assert(fvm_object_call(c,o,name,frame?1:0,frame?&a:0,&v)==FVM_OK); }
static FVM_SWFStreamOutputInfo observe(FVM_DesktopAudio *audio)
{ FVM_SWFStreamOutputInfo info;info.struct_size=sizeof(info);assert(fvm_desktop_audio_stream_info(audio,&info)==FVM_OK);return info; }
static void wait_frames(FVM_DesktopAudio *audio,double count)
{ for(unsigned i=0;i<200;++i){assert(SDL_AtomicGet(&audio->error)==FVM_OK);if(observe(audio).submitted_frames==count)return;SDL_Delay(5);}assert(false); }
int main()
{
 for(unsigned mode=0;mode<2;++mode) for(unsigned layout=0;layout<2;++layout) {
    if(mode && !fvm_jit_available())continue;
    FVM_Config cfg;FVM_Runtime *runtime=0;FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&runtime)==FVM_OK && fvm_context_create(runtime,&c)==FVM_OK);
    Bytes tags,head,pcm,empty;head.push_back(0x0e);head.push_back(0x3e);word(head,4410);tag(tags,45,head);
    for(unsigned i=0;i<4410;++i)word(pcm,1000);tag(tags,19,pcm);tag(tags,1,empty);
    if(layout){head.clear();head.push_back(0x0e);head.push_back(0x34);word(head,1102);tag(tags,45,head);}tag(tags,1,empty);
    pcm.clear();if(layout){head.clear();head.push_back(0x0e);head.push_back(0x3b);word(head,2205);tag(tags,45,head);}
    for(unsigned i=0;i<(layout?2205:4410);++i){word(pcm,(unsigned short)-2000);if(layout)word(pcm,3000);}tag(tags,19,pcm);tag(tags,1,empty);tag(tags,19,empty);
    Bytes swf=movie(tags,8);swf[12]=4;swf[13]=0;
    assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK && fvm_standard_install(c)==FVM_OK);
    assert(fvm_desktop_audio_validate(c)==FVM_OK);
    FVM_Object *clip=0;assert(fvm_movieclip_load_swf_ex(c,swf.begin(),swf.size(),"root",FVM_SWF_IMPORT_MUTE_ROOT_STREAMS,&clip)==FVM_OK && fvm_gc_pin(c,clip)==FVM_OK);
    assert(fvm_execution_set_mode(c,mode?FVM_EXECUTION_JIT:FVM_EXECUTION_INTERPRETER,0)==FVM_OK);
    FVM_DesktopAudio audio;assert(fvm_desktop_audio_open(c,clip,&audio)==FVM_OK && audio.thread && audio.stream_source && !audio.output && audio.stream_count==(layout?3U:1U));
    SDL_Delay(30);assert(observe(&audio).submitted_frames==0 && observe(&audio).paused);
    assert(fvm_movieclip_advance(c,clip,1)==FVM_OK && fvm_desktop_audio_timeline(c,clip,&audio)==FVM_OK);wait_frames(&audio,4800);
    SDL_Delay(130);assert(observe(&audio).submitted_frames==4800); /* Future frame3 held during slow host work. */
    method(c,clip,"stop");assert(fvm_desktop_audio_timeline(c,clip,&audio)==FVM_OK);FVM_SWFStreamOutputInfo before=observe(&audio);assert(before.paused);
    SDL_Delay(40);assert(observe(&audio).played_frames==before.played_frames);
    assert(fvm_movieclip_advance(c,clip,2)==FVM_OK && fvm_gc_step(c,0)==FVM_OK && fvm_desktop_audio_timeline(c,clip,&audio)==FVM_OK);
    method(c,clip,"play");assert(fvm_movieclip_advance(c,clip,1)==FVM_OK && fvm_desktop_audio_timeline(c,clip,&audio)==FVM_OK);
    SDL_Delay(20);assert(observe(&audio).submitted_frames==4800); /* Actual frame2 gap. */
    assert(fvm_movieclip_advance(c,clip,1)==FVM_OK && fvm_desktop_audio_timeline(c,clip,&audio)==FVM_OK);wait_frames(&audio,9600);
    assert(fvm_movieclip_advance(c,clip,1)==FVM_OK && fvm_desktop_audio_timeline(c,clip,&audio)==FVM_OK);
    for(unsigned i=0;i<100 && !observe(&audio).eof;++i)SDL_Delay(5);assert(observe(&audio).eof);
    assert(fvm_audio_stop_all(c)==FVM_OK && audio.stops==1 && observe(&audio).submitted_frames==0);
    SDL_Delay(20);assert(observe(&audio).submitted_frames==0);
    method(c,clip,"gotoAndPlay",1);assert(fvm_desktop_audio_timeline(c,clip,&audio)==FVM_OK);
    assert(fvm_desktop_audio_pause(&audio,1)==FVM_OK && fvm_movieclip_advance(c,clip,1)==FVM_OK && fvm_desktop_audio_timeline(c,clip,&audio)==FVM_OK);
    SDL_Delay(20);assert(observe(&audio).submitted_frames==0); /* Host/session pause wins over play. */
    assert(fvm_desktop_audio_tick(&audio)==FVM_OK);wait_frames(&audio,4800);
    /* Same-frame seek enters entirely between observations; token still forces queue reset. */
    assert(fvm_desktop_audio_pause(&audio,1)==FVM_OK);
    method(c,clip,"gotoAndPlay",1);assert(fvm_movieclip_advance(c,clip,1)==FVM_OK);
    assert(fvm_desktop_audio_timeline(c,clip,&audio)==FVM_OK && observe(&audio).submitted_frames==0);
    assert(fvm_desktop_audio_tick(&audio)==FVM_OK);wait_frames(&audio,4800);
    if(layout) {
        assert(fvm_desktop_audio_pause(&audio,1)==FVM_OK);method(c,clip,"gotoAndPlay",3);
        assert(fvm_desktop_audio_timeline(c,clip,&audio)==FVM_OK && audio.stream_index==2 && observe(&audio).submitted_frames==0);
        assert(fvm_movieclip_advance(c,clip,1)==FVM_OK && fvm_desktop_audio_timeline(c,clip,&audio)==FVM_OK && fvm_desktop_audio_tick(&audio)==FVM_OK);wait_frames(&audio,4800);
    }
    assert(fvm_desktop_audio_clear(c,&audio)==FVM_OK && !audio.thread && !audio.device && !audio.stream_source);
    assert(fvm_gc_unpin(c,clip)==FVM_OK);fvm_context_destroy(c);fvm_runtime_destroy(runtime);
 }
 return 0;
}
