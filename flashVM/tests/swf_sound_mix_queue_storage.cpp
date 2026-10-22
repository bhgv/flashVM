/* Requested bytes agree with fixed mono/stereo buffers and do not grow on rejected writes. */
#define main tail_reference_main
#include "swf_sound_mix_tail.cpp"
#undef main
int main()
{
 size_t required=91;assert(fvm_swf_sound_mix_queue_storage_required(0,2,&required)==FVM_INVALID_ARGUMENT && required==91);assert(fvm_swf_sound_mix_queue_storage_required(1,0,&required)==FVM_INVALID_ARGUMENT && required==91);assert(fvm_swf_sound_mix_queue_storage_required(1,1,0)==FVM_INVALID_ARGUMENT);
 for(unsigned channels=1;channels<=2;++channels){FVM_SWFSoundMixer *mixer=create(44100,channels);TailQueue q;std::memset(&q,0,sizeof(q));FVM_AudioQueueBackend b;std::memset(&b,0,sizeof(b));b.struct_size=sizeof(b);b.abi_version=FVM_AUDIO_QUEUE_ABI_VERSION;b.sample_rate=44100;b.channels=channels;b.userdata=&q;b.write=tail_write;b.queued=tail_count;b.pause=tail_pause;b.clear=tail_clear;FVM_AudioQueueBackend adapted;FVM_SWFSoundMixQueue *queue=0;
  assert(fvm_swf_sound_mix_queue_create(mixer,&b,16,&queue,&adapted)==FVM_OK && fvm_swf_sound_mix_queue_storage_required(channels,16,&required)==FVM_OK);FVM_SWFSoundMixQueueStorageInfo info;info.struct_size=sizeof(info);assert(fvm_swf_sound_mix_queue_storage_info(queue,&info)==FVM_OK && info.buffer_bytes==2*16*channels*sizeof(short) && info.retained_bytes==required && info.retained_bytes==info.fixed_bytes+info.buffer_bytes);
  short base[32]={0};q.fail=1;assert(adapted.write(adapted.userdata,base,16)==FVM_ERROR && fvm_swf_sound_mix_queue_storage_info(queue,&info)==FVM_OK && info.retained_bytes==required);info.struct_size=0;info.retained_bytes=91;assert(fvm_swf_sound_mix_queue_storage_info(queue,&info)==FVM_INVALID_ARGUMENT && info.retained_bytes==91);fvm_swf_sound_mix_queue_close(queue);fvm_swf_sound_mixer_close(mixer);
 }
 return 0;
}
