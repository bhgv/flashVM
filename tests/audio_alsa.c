/* Actual static ALSA null PCM write/drop/reprepare plus disabled-provider contracts. */
#include "flashvm/multimedia.h"
#include "flashvm/avm1.h"
#include <assert.h>
#include <string.h>
int main(void)
{
 FVM_AlsaOutput *device=(FVM_AlsaOutput *)1;FVM_MediaOutput output;
 FVM_Result result;memset(&output,1,sizeof(output));
 assert(fvm_audio_linux_alsa_open(0,8000,1,50000,&device,&output)==FVM_INVALID_ARGUMENT && !device && !output.write);
 assert(fvm_audio_linux_alsa_open("flashvm_null",0,1,50000,&device,&output)==FVM_INVALID_ARGUMENT && !device);
 assert(fvm_audio_linux_alsa_close(0)==FVM_OK);
 result=fvm_audio_linux_alsa_open("flashvm_null",8000,1,50000,&device,&output);
#ifdef FVM_TEST_LINUX_AUDIO
 assert(result==FVM_OK && device && output.write && output.stop);
 {
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *context=0;FVM_AudioChannel *channel=0;
 short samples[8]={0};FVM_MediaFrame frame;const unsigned char stop[]={9,0};
 memset(&frame,0,sizeof(frame));frame.kind=FVM_MEDIA_AUDIO;frame.data=(const unsigned char *)samples;frame.size=sizeof(samples);frame.samples=8;frame.channels=1;frame.sample_rate=8000;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&context)==FVM_OK);
 assert(fvm_audio_channel_create(context,&output,&channel)==FVM_OK);
 frame.sample_rate=16000;assert(fvm_audio_channel_write(channel,&frame)==FVM_INVALID_ARGUMENT);frame.sample_rate=8000;
 assert(fvm_audio_channel_write(channel,&frame)==FVM_OK);
 assert(fvm_avm1_execute(context,stop,sizeof(stop))==FVM_OK);
 assert(fvm_audio_channel_write(channel,&frame)==FVM_OK); /* Actual reprepare after drop. */
 assert(fvm_audio_channel_close(channel)==FVM_OK);
 fvm_context_destroy(context);fvm_runtime_destroy(rt);
 assert(fvm_audio_linux_alsa_close(device)==FVM_OK);
 device=0;assert(fvm_audio_linux_alsa_open("flashvm_no_such_pcm",8000,1,50000,&device,&output)==FVM_ERROR && !device && !output.write);
 }
#else
 assert(result==FVM_UNSUPPORTED && !device && !output.write && !output.stop && !output.userdata);
#endif
 return 0;
}
