/* Actual desktop acquisition routes root StartSound through the bounded stream composition. */
#include "../common/audio.h"
#include "../../tests/swf_sound_conversion_fixture.hpp"
#include "flashvm/movieclip.h"
#include "flashvm/gc.h"
#include "flashvm/standard.h"
using namespace conversion_fixture;
static Bytes mixed_movie()
{
 Bytes tags,head,pcm,empty;Samples samples;for(unsigned i=0;i<8;++i)samples.push_back(2000);definition(tags,7,3,1,samples);head.push_back(0x0e);head.push_back(0x3e);sound_fixture::word(head,4);sound_fixture::tag(tags,45,head);for(unsigned i=0;i<4;++i)sound_fixture::word(pcm,100);sound_fixture::tag(tags,19,pcm);event(tags,7);sound_fixture::tag(tags,1,empty);event(tags,7,FVM_SWF_SOUND_EVENT_NO_MULTIPLE);Bytes swf=sound_fixture::movie(tags);swf[12]=2;return swf;
}
#ifndef FVM_DESKTOP_MIXED_FIXTURE_ONLY
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);Bytes swf=mixed_movie();assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK && fvm_desktop_audio_validate(c)==FVM_OK);FVM_Object *clip=0;assert(fvm_movieclip_load_swf_ex(c,swf.begin(),swf.size(),"root",FVM_SWF_IMPORT_MUTE_ROOT_STREAMS,&clip)==FVM_OK && fvm_gc_pin(c,clip)==FVM_OK);FVM_DesktopAudio a;assert(fvm_desktop_audio_open_paused(c,clip,&a,1)==FVM_OK && a.composition && a.mixer && !a.output);assert(fvm_movieclip_advance(c,clip,1)==FVM_OK && a.events==1);assert(SDL_LockMutex(a.mutex)==0);size_t n=0;assert(fvm_swf_stream_output_pump_until(&a.stream_output,1,&n)==FVM_OK && n==5);int yes=0;assert(fvm_swf_sound_mix_queue_sound_active(a.composition,7,&yes)==FVM_OK && yes);FVM_SWFSoundMixQueueOriginInfo origins;origins.struct_size=sizeof(origins);assert(fvm_swf_sound_mix_queue_origin_info(a.composition,&origins)==FVM_OK&&origins.base_submitted_frames==5&&!origins.tail_submitted_frames&&origins.base_queued_frames==5);SDL_UnlockMutex(a.mutex);FVM_SWFStreamMixInfo paired;paired.struct_size=sizeof(paired);assert(fvm_desktop_audio_mix_info(&a,&paired)==FVM_OK&&paired.stream.submitted_frames==5&&paired.stream.queued_frames==5&&paired.origins.base_queued_frames==5&&!paired.origins.tail_queued_frames);FVM_SWFStreamMixInfo saved=paired;paired.struct_size=0;assert(fvm_desktop_audio_mix_info(&a,&paired)==FVM_INVALID_ARGUMENT);paired=saved;assert(fvm_desktop_audio_mix_info(0,&paired)==FVM_INVALID_ARGUMENT&&!memcmp(&saved,&paired,sizeof(saved)));assert(fvm_gc_collect(c,0)==FVM_OK);assert(fvm_movieclip_advance(c,clip,1)==FVM_OK && a.events==2);assert(fvm_movieclip_set_sound_callback(c,0,0)==FVM_OK);fvm_desktop_audio_close(&a);fvm_gc_unpin(c,clip);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
#endif
