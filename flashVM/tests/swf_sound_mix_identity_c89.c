/* Real C89 tracked ownership and copied error outputs after source VM teardown. */
#define main identity_tutorial_reference_main
#include "../examples/multimedia/swf_stream_mix/main.c"
#undef main
int main(void)
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_SWFSoundVoice *voice[2]={0,0};FVM_SWFSoundMixer *mixer=0;FVM_SWFSoundMixQueue *queue=0;FVM_AudioQueueBackend b,adapted;ExampleQueue fifo;FVM_SWFSoundMixQueueIdentityInfo info,saved;size_t bytes=0,n=0;int yes=91;
 if(fvm_config_init(&cfg)!=FVM_OK || fvm_runtime_create(&cfg,&rt)!=FVM_OK || fvm_context_create(rt,&c)!=FVM_OK || fvm_load_memory(c,movie,sizeof(movie))!=FVM_OK || fvm_swf_sound_voice_open_bounded(c,0,(size_t)-1,voice)!=FVM_OK || fvm_swf_sound_voice_open_bounded(c,1,(size_t)-1,voice+1)!=FVM_OK)return 1;
 fvm_context_destroy(c);fvm_runtime_destroy(rt);memset(&fifo,0,sizeof(fifo));memset(&b,0,sizeof(b));b.struct_size=sizeof(b);b.abi_version=FVM_AUDIO_QUEUE_ABI_VERSION;b.sample_rate=48000;b.channels=2;b.userdata=&fifo;b.write=queue_write;b.queued=queue_count;b.pause=queue_pause;b.clear=queue_clear;
 {FVM_SWFSoundMixerConfig geometry;geometry.struct_size=sizeof(geometry);geometry.sample_rate=48000;geometry.channels=2;geometry.voice_capacity=2;geometry.flags=FVM_SWF_SOUND_MIXER_CONVERT;if(fvm_swf_sound_mixer_create_configured(&geometry,&mixer)!=FVM_OK)return 1;}
 if(fvm_swf_sound_mix_queue_create(mixer,&b,2,&queue,&adapted)!=FVM_OK || fvm_swf_sound_mix_queue_clock_begin(queue)!=FVM_OK || fvm_swf_sound_mix_queue_storage_required_for_tracking(2,2,1,&bytes)!=FVM_OK || fvm_swf_sound_mix_queue_set_identity_tracking(queue,1,bytes)!=FVM_OK)return 1;
 if(fvm_swf_sound_mix_queue_submit_prepared_bounded(queue,voice,(size_t)-1)!=FVM_OK || voice[0] || fvm_swf_sound_mix_queue_sound_active(queue,7,&yes)!=FVM_OK || !yes)return 1;
 memset(&info,0x5a,sizeof(info));info.struct_size=sizeof(info);saved=info;if(fvm_swf_sound_mix_queue_identity_info(queue,0,&info)!=FVM_INVALID_ARGUMENT || memcmp(&info,&saved,sizeof(info)))return 1;
 if(fvm_swf_sound_mix_queue_identity_info(queue,7,&info)!=FVM_OK || info.end_frame!=9 || !info.active || fvm_swf_sound_mix_queue_stop_id(queue,7)!=FVM_OK || fvm_swf_sound_mix_queue_sound_active(queue,7,&yes)!=FVM_OK || yes)return 1;
 if(fvm_swf_sound_mix_queue_submit_prepared_bounded(queue,voice+1,(size_t)-1)!=FVM_OK || voice[1] || fvm_swf_sound_mix_queue_pump_tail(queue,2,8,&n)!=FVM_OK || n!=2 || fvm_swf_sound_mix_queue_stop_all(queue)!=FVM_OK || adapted.clear(adapted.userdata)!=FVM_OK)return 1;
 if(fvm_swf_sound_mix_queue_identity_info(queue,8,&info)!=FVM_OK || info.end_frame || info.active || fvm_swf_sound_mix_queue_set_identity_tracking(queue,0,0)!=FVM_OK || fvm_swf_sound_mix_queue_clock_end(queue)!=FVM_OK)return 1;
 saved=info;if(fvm_swf_sound_mix_queue_identity_info(queue,8,&info)!=FVM_UNSUPPORTED || memcmp(&info,&saved,sizeof(info)))return 1;
 fvm_swf_sound_mix_queue_close(queue);fvm_swf_sound_mixer_close(mixer);return 0;
}
