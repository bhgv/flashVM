/* Four real OS workers independently begin drain and end shared composition clock epochs. */
#define main tail_reference_main
#include "swf_sound_mix_tail.cpp"
#undef main
#include "../ports/thread_port.h"
struct TailJob {unsigned completed;short first;};
static void tail_worker(void *raw)
{
 TailJob *job=(TailJob*)raw;FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);Samples samples;for(unsigned i=0;i<8;++i)samples.push_back((short)(job->first+i));Bytes swf=movie(samples,3,1);assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);FVM_SWFSoundVoice *source=0;assert(fvm_swf_sound_voice_open(c,0,&source)==FVM_OK);fvm_context_destroy(c);fvm_runtime_destroy(rt);
 FVM_SWFSoundMixer *mixer=create(44100,1);TailQueue q;std::memset(&q,0,sizeof(q));FVM_AudioQueueBackend b;std::memset(&b,0,sizeof(b));b.struct_size=sizeof(b);b.abi_version=FVM_AUDIO_QUEUE_ABI_VERSION;b.sample_rate=44100;b.channels=1;b.userdata=&q;b.write=tail_write;b.queued=tail_count;b.pause=tail_pause;b.clear=tail_clear;FVM_AudioQueueBackend adapted;FVM_SWFSoundMixQueue *queue=0;size_t budget=0;assert(fvm_swf_sound_mix_queue_storage_required(1,2,&budget)==FVM_OK && fvm_swf_sound_mix_queue_create_bounded(mixer,&b,2,budget,&queue,&adapted)==FVM_OK);
 for(unsigned iteration=0;iteration<64;++iteration){FVM_SWFSoundVoice *voice=0;assert(fvm_swf_sound_voice_clone(source,(size_t)-1,&voice)==FVM_OK && fvm_swf_sound_mixer_submit_prepared(mixer,&voice)==FVM_OK && !voice);q.total=q.count=0;assert(fvm_swf_sound_mix_queue_clock_begin(queue)==FVM_OK);for(unsigned block=0;block<4;++block){q.count=0;q.fail=1;size_t accepted=91;assert(fvm_swf_sound_mix_queue_pump_tail(queue,2,4,&accepted)==FVM_ERROR && !accepted);q.fail=0;assert(fvm_swf_sound_mix_queue_pump_tail(queue,2,4,&accepted)==FVM_OK && accepted==2);}for(unsigned i=0;i<8;++i)assert(q.saved[i]==job->first+i);assert(fvm_swf_sound_mix_queue_pump_tail(queue,2,4,0)==FVM_NOT_FOUND);FVM_SWFSoundMixQueueClock clock;clock.struct_size=sizeof(clock);assert(fvm_swf_sound_mix_queue_clock_info(queue,&clock)==FVM_OK && clock.submitted_frames==8 && clock.played_frames==6 && clock.queued_frames==2);q.count=0;assert(fvm_swf_sound_mix_queue_clock_info(queue,&clock)==FVM_OK && clock.played_frames==8 && clock.seconds==8.0/44100 && fvm_swf_sound_mix_queue_clock_end(queue)==FVM_OK);++job->completed;}
 fvm_swf_sound_mix_queue_close(queue);fvm_swf_sound_mixer_close(mixer);fvm_swf_sound_voice_close(source);
}
int main()
{
 TailJob jobs[4];FVM_PortThread *threads[4];for(unsigned i=0;i<4;++i){jobs[i].completed=0;jobs[i].first=(short)(100+i*100);threads[i]=fvm_port_thread_start(tail_worker,jobs+i);assert(threads[i]);}for(unsigned i=0;i<4;++i){fvm_port_thread_join(threads[i]);assert(jobs[i].completed==64);}return 0;
}
