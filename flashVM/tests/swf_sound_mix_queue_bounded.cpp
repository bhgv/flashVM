/* Exact-fit creation succeeds; insufficient budgets and invalid descriptors publish no ownership. */
#define main tail_reference_main
#include "swf_sound_mix_tail.cpp"
#undef main
int main()
{
 FVM_SWFSoundMixer *mixer=create(44100,1);TailQueue q;std::memset(&q,0,sizeof(q));FVM_AudioQueueBackend b;std::memset(&b,0,sizeof(b));b.struct_size=sizeof(b);b.abi_version=FVM_AUDIO_QUEUE_ABI_VERSION;b.sample_rate=44100;b.channels=1;b.userdata=&q;b.write=tail_write;b.queued=tail_count;b.pause=tail_pause;b.clear=tail_clear;FVM_AudioQueueBackend adapted=b;FVM_SWFSoundMixQueue *queue=(FVM_SWFSoundMixQueue*)1;size_t required=0;assert(fvm_swf_sound_mix_queue_storage_required(1,16,&required)==FVM_OK && required);
 assert(fvm_swf_sound_mix_queue_create_bounded(mixer,&b,16,0,&queue,&adapted)==FVM_OUT_OF_MEMORY && !queue && !std::memcmp(&b,&adapted,sizeof(b)) && !q.calls);
 assert(fvm_swf_sound_mix_queue_create_bounded(mixer,&b,16,required-1,&queue,&adapted)==FVM_OUT_OF_MEMORY && !queue && !std::memcmp(&b,&adapted,sizeof(b)) && !q.calls);
 assert(fvm_swf_sound_mix_queue_create_bounded(mixer,&b,0,0,&queue,&adapted)==FVM_INVALID_ARGUMENT && !queue);
 b.channels=2;assert(fvm_swf_sound_mix_queue_create_bounded(mixer,&b,16,0,&queue,&adapted)==FVM_INVALID_ARGUMENT && !queue);b.channels=1;
 assert(fvm_swf_sound_mix_queue_create_bounded(mixer,&b,16,required,&queue,&adapted)==FVM_OK && queue && !q.calls);FVM_SWFSoundMixQueueStorageInfo info;info.struct_size=sizeof(info);assert(fvm_swf_sound_mix_queue_storage_info(queue,&info)==FVM_OK && info.retained_bytes==required);fvm_swf_sound_mix_queue_close(queue);fvm_swf_sound_mixer_close(mixer);
 return 0;
}
