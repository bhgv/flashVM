/* Actual DefineSound -> native-S16 conversion -> static ALSA null PCM -> StopSounds. */
#include "flashvm/swf_sound.h"
#include "flashvm/avm1.h"
#include "swf_sound_fixture.hpp"
using namespace sound_fixture;
int main()
{
 FVM_AlsaOutput *device=0;FVM_MediaOutput output;FVM_Result result=fvm_audio_linux_alsa_open("flashvm_null",44100,1,50000,&device,&output);
#ifdef FVM_TEST_LINUX_AUDIO
 assert(result==FVM_OK && device);
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
 const unsigned char pcm[]={0,128,255,127,0,0,1,0,254,255,255,255};Bytes definitions;tag(definitions,14,sound(7,0x3e,6,pcm,sizeof(pcm)));Bytes event;word(event,7);event.push_back(15);integer(event,0);integer(event,5);word(event,2);event.push_back(2);integer(event,0);word(event,32768);word(event,32768);integer(event,5);word(event,16384);word(event,16384);tag(definitions,15,event);Bytes swf=movie(definitions);assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);
 FVM_AudioChannel *channel=0;assert(fvm_audio_channel_create(c,&output,&channel)==FVM_OK);
 assert(fvm_swf_sound_submit(c,7,channel,1,3)==FVM_OK);const unsigned char stop[]={9,0};assert(fvm_avm1_execute(c,stop,sizeof(stop))==FVM_OK);
 assert(fvm_swf_sound_event_submit(c,0,channel)==FVM_OK);assert(fvm_avm1_execute(c,stop,sizeof(stop))==FVM_OK);
 assert(fvm_swf_sound_submit(c,7,channel,0,6)==FVM_OK);assert(fvm_audio_channel_close(channel)==FVM_OK);
 fvm_context_destroy(c);fvm_runtime_destroy(rt);assert(fvm_audio_linux_alsa_close(device)==FVM_OK);
#else
 assert(result==FVM_UNSUPPORTED && !device && !output.write);
#endif
 return 0;
}
