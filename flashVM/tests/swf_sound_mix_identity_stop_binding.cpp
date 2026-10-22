/* Actual StopSounds clears the whole composition; event-only stop_all deliberately retains FIFO. */
#define FVM_MOVIECLIP_SOUND_FIXTURE_ONLY
#include "movieclip_sound.cpp"
#include "swf_sound_output_fixture.hpp"
#include "flashvm/audio_stop.h"
static Bytes controlled_movie()
{
    Bytes tags,pcm,none,stop_action;
    u16(pcm,7);pcm.push_back(12);u32(pcm,2);pcm.push_back(0);pcm.push_back(255);
    tag(tags,14,pcm);
    stop_action.push_back(9);stop_action.push_back(0);
    sound_event(tags,0);tag(tags,12,stop_action);tag(tags,1,none);
    sound_event(tags,0);tag(tags,12,action(2));tag(tags,1,none);
    tag(tags,12,stop_action);tag(tags,1,none);
    sound_event(tags,0);tag(tags,12,action(4));tag(tags,1,none);tag(tags,0,none);
    return movie(tags,4);
}

#include "flashvm/swf_sound_mix_queue.h"
struct MixedControl {FVM_SWFSoundMixQueue *queue;FVM_AudioQueueBackend adapted;unsigned calls;};
static FVM_Result mixed_stop(void *raw)
{
 MixedControl *h=(MixedControl*)raw;++h->calls;return h->adapted.clear(h->adapted.userdata);
}
static FVM_Result mixed_receive(FVM_Context *c,FVM_Object *,size_t ordinal,void *raw)
{
 MixedControl *h=(MixedControl*)raw;FVM_SWFSoundVoice *v=0;FVM_Result r=fvm_swf_sound_voice_open(c,ordinal,&v);if(r==FVM_OK)r=fvm_swf_sound_mix_queue_submit_prepared_bounded(h->queue,&v,(size_t)-1);fvm_swf_sound_voice_close(v);return r;
}
int main()
{
 unsigned executed=0;
 for(unsigned mode=0;mode<2;++mode){if(mode&&!fvm_jit_available())continue;
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);Bytes swf=controlled_movie();assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);FVM_Object *clip=0;assert(fvm_movieclip_load_swf(c,swf.begin(),swf.size(),"root",&clip)==FVM_OK && fvm_gc_pin(c,clip)==FVM_OK && fvm_execution_set_mode(c,mode?FVM_EXECUTION_JIT:FVM_EXECUTION_INTERPRETER,0)==FVM_OK);
 FVM_SWFSoundMixer *mixer=conversion_fixture::create(48000,2);output_fixture::Device device;FVM_AudioQueueBackend real=output_fixture::backend(device);MixedControl h;h.queue=0;h.calls=0;size_t bytes=0;assert(fvm_swf_sound_mix_queue_storage_required_for_tracking(2,4,1,&bytes)==FVM_OK && fvm_swf_sound_mix_queue_create(mixer,&real,4,&h.queue,&h.adapted)==FVM_OK && fvm_swf_sound_mix_queue_clock_begin(h.queue)==FVM_OK && fvm_swf_sound_mix_queue_set_identity_tracking(h.queue,1,bytes)==FVM_OK);FVM_AudioStopBinding *binding=0;assert(fvm_audio_stop_binding_create(c,mixed_stop,&h,&binding)==FVM_OK && fvm_movieclip_set_sound_callback(c,mixed_receive,&h)==FVM_OK);
 assert(fvm_movieclip_advance(c,clip,1)==FVM_OK && h.calls==1);FVM_SWFSoundMixQueueIdentityInfo id;id.struct_size=sizeof(id);assert(fvm_swf_sound_mix_queue_identity_info(h.queue,7,&id)==FVM_OK && !id.end_frame);
 assert(fvm_movieclip_advance(c,clip,1)==FVM_OK);size_t n=0;device.failure=1;assert(fvm_swf_sound_mix_queue_pump_tail(h.queue,4,16,&n)==FVM_ERROR);device.failure=4;assert(fvm_audio_stop_all(c)==FVM_ERROR && h.calls==2);FVM_SWFSoundMixQueuePending pending;pending.struct_size=sizeof(pending);assert(fvm_swf_sound_mix_queue_pending(h.queue,&pending)==FVM_OK && pending.frames==4);assert(fvm_swf_sound_mix_queue_identity_info(h.queue,7,&id)==FVM_OK && id.end_frame==3);assert(fvm_gc_step(c,0)==FVM_OK);
 device.failure=0;assert(fvm_movieclip_advance(c,clip,1)==FVM_OK && h.calls==3);assert(fvm_swf_sound_mix_queue_pending(h.queue,&pending)==FVM_OK && !pending.frames);assert(fvm_movieclip_advance(c,clip,1)==FVM_OK && fvm_swf_sound_mix_queue_pump_tail(h.queue,4,16,&n)==FVM_OK && n==4);assert(fvm_swf_sound_mix_queue_stop_all(h.queue)==FVM_OK && device.queued==4);assert(fvm_swf_sound_mix_queue_identity_info(h.queue,7,&id)==FVM_OK && !id.end_frame);
 FVM_ExecutionStats stats;stats.struct_size=sizeof(stats);assert(fvm_execution_get_stats(c,&stats)==FVM_OK);assert(mode?stats.native_entries>0:stats.interpreter_entries>0&&!stats.native_entries);assert(fvm_gc_unpin(c,clip)==FVM_OK);fvm_context_destroy(c);assert(h.calls==4 && !device.queued);fvm_runtime_destroy(rt);fvm_swf_sound_mix_queue_close(h.queue);fvm_swf_sound_mixer_close(mixer);++executed;}
 std::printf("PASS mixed StopSounds binding: interpreter=1 native_jit=%u\n",executed>1?1U:0U);return 0;
}
