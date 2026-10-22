/* White-box seed of the exact integer limit avoids pretending to play 2^53 frames in a test. */
#include "../src/swf/sound_mix_queue.cpp"
#define main tail_reference_main
#include "swf_sound_mix_tail.cpp"
#undef main
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);Samples samples;for(unsigned i=0;i<4;++i)samples.push_back((short)(100+i));Bytes swf=movie(samples,3,1);assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);FVM_SWFSoundMixer *mixer=create(44100,1);assert(fvm_swf_sound_mixer_submit(mixer,c,0)==FVM_OK);fvm_context_destroy(c);fvm_runtime_destroy(rt);
 TailQueue q;std::memset(&q,0,sizeof(q));FVM_AudioQueueBackend b;std::memset(&b,0,sizeof(b));b.struct_size=sizeof(b);b.abi_version=FVM_AUDIO_QUEUE_ABI_VERSION;b.sample_rate=44100;b.channels=1;b.userdata=&q;b.write=tail_write;b.queued=tail_count;b.pause=tail_pause;b.clear=tail_clear;FVM_AudioQueueBackend adapted;FVM_SWFSoundMixQueue *queue=0;assert(fvm_swf_sound_mix_queue_create(mixer,&b,2,&queue,&adapted)==FVM_OK && fvm_swf_sound_mix_queue_clock_begin(queue)==FVM_OK);
 queue->clock_submitted=queue->clock_played=FVM_SWF_SOUND_OUTPUT_MAX_CLOCK-2;short zero[2]={0,0};q.fail=1;assert(adapted.write(adapted.userdata,zero,2)==FVM_ERROR && queue->clock_submitted==FVM_SWF_SOUND_OUTPUT_MAX_CLOCK-2);q.fail=0;assert(adapted.write(adapted.userdata,zero,2)==FVM_OK && queue->clock_submitted==FVM_SWF_SOUND_OUTPUT_MAX_CLOCK);FVM_SWFSoundMixQueueClock clock;clock.struct_size=sizeof(clock);assert(fvm_swf_sound_mix_queue_clock_info(queue,&clock)==FVM_OK && clock.submitted_frames==FVM_SWF_SOUND_OUTPUT_MAX_CLOCK && clock.played_frames==FVM_SWF_SOUND_OUTPUT_MAX_CLOCK-2);
 size_t calls=q.calls,accepted=91;assert(adapted.write(adapted.userdata,zero,1)==FVM_UNSUPPORTED && q.calls==calls);assert(fvm_swf_sound_mix_queue_pump_tail(queue,1,4,&accepted)==FVM_UNSUPPORTED && !accepted && q.calls==calls);FVM_SWFSoundMixQueuePending pending;pending.struct_size=sizeof(pending);assert(fvm_swf_sound_mix_queue_pending(queue,&pending)==FVM_OK && !pending.frames);assert(adapted.clear(adapted.userdata)==FVM_OK && fvm_swf_sound_mix_queue_pump_tail(queue,1,4,&accepted)==FVM_OK && accepted==1 && q.saved[2]==102);assert(fvm_swf_sound_mix_queue_clock_info(queue,&clock)==FVM_OK && clock.submitted_frames==1 && clock.played_frames==0);
 fvm_swf_sound_mix_queue_close(queue);fvm_swf_sound_mixer_close(mixer);return 0;
}
