/* Independent native templates and a tracked composition FIFO, all after source VM teardown. */
#ifndef FVM_TEST_MIX_IDENTITY_FIXTURE_HPP
#define FVM_TEST_MIX_IDENTITY_FIXTURE_HPP
#define main identity_tail_reference_main
#include "swf_sound_mix_tail.cpp"
#undef main
struct IdentityFixture
{
 FVM_SWFSoundMixer *mixer;FVM_SWFSoundMixQueue *queue;FVM_SWFSoundVoice *source[2];TailQueue fifo;FVM_AudioQueueBackend adapted;size_t adapter_budget;
 IdentityFixture(bool tracking=true)
 {
  mixer=0;queue=0;source[0]=source[1]=0;FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);Samples samples;for(unsigned i=0;i<5;++i)samples.push_back((short)(100+i));Bytes swf=movie(samples,3,1);assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);for(unsigned i=0;i<2;++i)assert(fvm_swf_sound_voice_open(c,i,source+i)==FVM_OK);fvm_context_destroy(c);fvm_runtime_destroy(rt);mixer=create(44100,1);std::memset(&fifo,0,sizeof(fifo));FVM_AudioQueueBackend b;std::memset(&b,0,sizeof(b));b.struct_size=sizeof(b);b.abi_version=FVM_AUDIO_QUEUE_ABI_VERSION;b.sample_rate=44100;b.channels=1;b.userdata=&fifo;b.write=tail_write;b.queued=tail_count;b.pause=tail_pause;b.clear=tail_clear;assert(fvm_swf_sound_mix_queue_create(mixer,&b,2,&queue,&adapted)==FVM_OK && fvm_swf_sound_mix_queue_clock_begin(queue)==FVM_OK);size_t base=0;assert(fvm_swf_sound_mix_queue_storage_required(1,2,&base)==FVM_OK);adapter_budget=base+FVM_SWF_SOUND_MIX_QUEUE_ID_SLOTS*sizeof(double);if(tracking)assert(fvm_swf_sound_mix_queue_set_identity_tracking(queue,1,adapter_budget)==FVM_OK);
 }
 ~IdentityFixture(){fvm_swf_sound_voice_close(source[0]);fvm_swf_sound_voice_close(source[1]);fvm_swf_sound_mix_queue_close(queue);fvm_swf_sound_mixer_close(mixer);}
 FVM_SWFSoundVoice *clone(unsigned which){FVM_SWFSoundVoice *voice=0;assert(fvm_swf_sound_voice_clone(source[which],(size_t)-1,&voice)==FVM_OK);return voice;}
};
#endif
