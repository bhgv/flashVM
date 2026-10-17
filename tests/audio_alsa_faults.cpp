/* Native ALSA error/short-write/drop/reprepare behavior, using its actual null PCM endpoint. */
#include "flashvm/multimedia.h"
#include <assert.h>
#include <string.h>
#ifdef FVM_TEST_LINUX_AUDIO
#include <alsa/asoundlib.h>
#include <errno.h>
static unsigned writes,drops,prepares,starts;
static unsigned mode;
extern "C" snd_pcm_sframes_t __real_snd_pcm_writei(snd_pcm_t *,const void *,snd_pcm_uframes_t);
extern "C" int __real_snd_pcm_drop(snd_pcm_t *);
extern "C" int __real_snd_pcm_prepare(snd_pcm_t *);
extern "C" int __real_snd_pcm_start(snd_pcm_t *);
extern "C" snd_pcm_sframes_t __wrap_snd_pcm_writei(snd_pcm_t *pcm,const void *data,snd_pcm_uframes_t count)
{
 ++writes;
 if(mode==1){mode=2;return __real_snd_pcm_writei(pcm,data,1);}
 if(mode==2){mode=0;return -EAGAIN;}
 if(mode==3){mode=0;return 0;}
 return __real_snd_pcm_writei(pcm,data,count);
}
extern "C" int __wrap_snd_pcm_drop(snd_pcm_t *pcm){++drops;return __real_snd_pcm_drop(pcm);}
extern "C" int __wrap_snd_pcm_prepare(snd_pcm_t *pcm){++prepares;return __real_snd_pcm_prepare(pcm);}
extern "C" int __wrap_snd_pcm_start(snd_pcm_t *pcm){++starts;if(mode==4){mode=0;return -EIO;}return __real_snd_pcm_start(pcm);}
#endif
int main()
{
#ifdef FVM_TEST_LINUX_AUDIO
 FVM_AlsaOutput *device=0;FVM_MediaOutput output;FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *context=0;FVM_AudioChannel *channel=0;
 assert(fvm_audio_linux_alsa_open("flashvm_null",8000,1,50000,&device,&output)==FVM_OK);
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&context)==FVM_OK);
 assert(fvm_audio_channel_create(context,&output,&channel)==FVM_OK);
 short samples[8]={0};FVM_MediaFrame frame;memset(&frame,0,sizeof(frame));frame.kind=FVM_MEDIA_AUDIO;frame.data=(const unsigned char *)samples;frame.size=sizeof(samples);frame.samples=8;frame.channels=1;frame.sample_rate=8000;
 int active=-1;unsigned before=drops;mode=1;assert(fvm_audio_channel_write(channel,&frame)==FVM_ERROR && writes==2 && drops==before+1);
 assert(fvm_audio_channel_active(channel,&active)==FVM_OK && !active);
 before=prepares;assert(fvm_audio_channel_write(channel,&frame)==FVM_OK && prepares>before && starts);
 before=drops;mode=3;assert(fvm_audio_channel_write(channel,&frame)==FVM_ERROR && drops==before+1);
 assert(fvm_audio_channel_active(channel,&active)==FVM_OK && active); /* Failed write retains prior acknowledgement. */
 assert(fvm_audio_channel_stop(channel)==FVM_OK);
 before=drops;mode=4;assert(fvm_audio_channel_write(channel,&frame)==FVM_ERROR && drops==before+1);
 assert(fvm_audio_channel_active(channel,&active)==FVM_OK && !active);
 assert(fvm_audio_channel_write(channel,&frame)==FVM_OK);
 assert(fvm_audio_channel_close(channel)==FVM_OK);
 fvm_context_destroy(context);fvm_runtime_destroy(rt);assert(fvm_audio_linux_alsa_close(device)==FVM_OK);
#endif
 return 0;
}
