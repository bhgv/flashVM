/* Shared accounting counts only accepted base/tail blocks and observes actual FIFO dequeue. */
#define main tail_reference_main
#include "swf_sound_mix_tail.cpp"
#undef main
int main()
{
 FVM_SWFSoundMixer *mixer=create(44100,1);TailQueue q;std::memset(&q,0,sizeof(q));FVM_AudioQueueBackend b;std::memset(&b,0,sizeof(b));b.struct_size=sizeof(b);b.abi_version=FVM_AUDIO_QUEUE_ABI_VERSION;b.sample_rate=44100;b.channels=1;b.userdata=&q;b.write=tail_write;b.queued=tail_count;b.pause=tail_pause;b.clear=tail_clear;FVM_AudioQueueBackend adapted;FVM_SWFSoundMixQueue *queue=0;assert(fvm_swf_sound_mix_queue_create(mixer,&b,2,&queue,&adapted)==FVM_OK);
 FVM_SWFSoundMixQueueClock clock;clock.struct_size=sizeof(clock);clock.submitted_frames=91;assert(fvm_swf_sound_mix_queue_clock_info(queue,&clock)==FVM_NOT_FOUND && clock.submitted_frames==91);assert(fvm_swf_sound_mix_queue_clock_begin(queue)==FVM_OK);short pcm[]={11,12};q.fail=1;assert(adapted.write(adapted.userdata,pcm,2)==FVM_ERROR);assert(fvm_swf_sound_mix_queue_clock_info(queue,&clock)==FVM_OK && clock.submitted_frames==0 && clock.played_frames==0);assert(fvm_swf_sound_mix_queue_clock_begin(queue)==FVM_INVALID_ARGUMENT);q.fail=0;assert(adapted.write(adapted.userdata,pcm,2)==FVM_OK);assert(fvm_swf_sound_mix_queue_clock_info(queue,&clock)==FVM_OK && clock.submitted_frames==2 && clock.queued_frames==2 && clock.played_frames==0);q.count=1;assert(fvm_swf_sound_mix_queue_clock_info(queue,&clock)==FVM_OK && clock.played_frames==1 && clock.seconds==1.0/44100);q.count=0;assert(fvm_swf_sound_mix_queue_clock_info(queue,&clock)==FVM_OK && clock.played_frames==2);assert(fvm_swf_sound_mix_queue_clock_begin(queue)==FVM_OK && fvm_swf_sound_mix_queue_clock_info(queue,&clock)==FVM_OK && !clock.submitted_frames);
 fvm_swf_sound_mix_queue_close(queue);fvm_swf_sound_mixer_close(mixer);return 0;
}
