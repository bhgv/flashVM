/* Clock epochs reject callback reentry and preserve observations across failures and queue regression. */
#define main tail_reference_main
#include "swf_sound_mix_tail.cpp"
#undef main
struct ClockQueue : TailQueue {FVM_SWFSoundMixQueue *owner;int bad_observe,bad_clear,bad_pause,throwing;};
static void clock_recursive(ClockQueue *q){FVM_SWFSoundMixQueueClock c;c.struct_size=sizeof(c);c.submitted_frames=91;assert(fvm_swf_sound_mix_queue_clock_begin(q->owner)==FVM_INVALID_ARGUMENT && fvm_swf_sound_mix_queue_clock_end(q->owner)==FVM_INVALID_ARGUMENT);assert(fvm_swf_sound_mix_queue_clock_info(q->owner,&c)==FVM_INVALID_ARGUMENT && c.submitted_frames==91);}
static FVM_Result clock_write(void *raw,const short *pcm,size_t frames){ClockQueue *q=(ClockQueue*)raw;clock_recursive(q);if(q->throwing==1)throw 7;return tail_write(q,pcm,frames);}
static FVM_Result clock_count(void *raw,size_t *out){ClockQueue *q=(ClockQueue*)raw;clock_recursive(q);if(q->throwing==2)throw 7;*out=q->count;return q->bad_observe?FVM_ERROR:FVM_OK;}
static FVM_Result clock_clear(void *raw){ClockQueue *q=(ClockQueue*)raw;clock_recursive(q);return q->bad_clear?FVM_ERROR:tail_clear(q);}
static FVM_Result clock_pause(void *raw,int){ClockQueue *q=(ClockQueue*)raw;clock_recursive(q);return q->bad_pause?FVM_ERROR:FVM_OK;}
int main(int argc,char **argv)
{
 bool exceptions=argc==1;assert(argc==1 || (argc==2 && !std::strcmp(argv[1],"--no-throw")));
 FVM_SWFSoundMixer *mixer=create(44100,1);ClockQueue q;std::memset(&q,0,sizeof(q));FVM_AudioQueueBackend b;std::memset(&b,0,sizeof(b));b.struct_size=sizeof(b);b.abi_version=FVM_AUDIO_QUEUE_ABI_VERSION;b.sample_rate=44100;b.channels=1;b.userdata=&q;b.write=clock_write;b.queued=clock_count;b.pause=clock_pause;b.clear=clock_clear;FVM_AudioQueueBackend adapted;assert(fvm_swf_sound_mix_queue_create(mixer,&b,2,&q.owner,&adapted)==FVM_OK);
 FVM_SWFSoundMixQueueClock c;c.struct_size=sizeof(c);c.submitted_frames=91;q.bad_observe=1;assert(fvm_swf_sound_mix_queue_clock_begin(q.owner)==FVM_ERROR && fvm_swf_sound_mix_queue_clock_info(q.owner,&c)==FVM_NOT_FOUND && c.submitted_frames==91);q.bad_observe=0;assert(fvm_swf_sound_mix_queue_clock_begin(q.owner)==FVM_OK);short pcm[]={7,8};q.fail=1;assert(adapted.write(adapted.userdata,pcm,2)==FVM_ERROR);q.bad_clear=q.bad_pause=1;assert(adapted.clear(adapted.userdata)==FVM_ERROR && adapted.pause(adapted.userdata,1)==FVM_ERROR);assert(fvm_swf_sound_mix_queue_clock_info(q.owner,&c)==FVM_OK && !c.submitted_frames);q.bad_clear=q.bad_pause=0;
 if(exceptions){q.throwing=1;assert(adapted.write(adapted.userdata,pcm,2)==FVM_ERROR);q.throwing=2;c.submitted_frames=91;assert(fvm_swf_sound_mix_queue_clock_info(q.owner,&c)==FVM_ERROR && c.submitted_frames==91);q.throwing=0;}
 q.fail=0;assert(adapted.write(adapted.userdata,pcm,2)==FVM_OK);q.count=0;assert(fvm_swf_sound_mix_queue_clock_info(q.owner,&c)==FVM_OK && c.submitted_frames==2 && c.played_frames==2);q.count=1;c.submitted_frames=91;assert(fvm_swf_sound_mix_queue_clock_info(q.owner,&c)==FVM_ERROR && c.submitted_frames==91);q.count=3;assert(fvm_swf_sound_mix_queue_clock_info(q.owner,&c)==FVM_ERROR && c.submitted_frames==91);q.count=0;assert(fvm_swf_sound_mix_queue_clock_info(q.owner,&c)==FVM_OK && c.played_frames==2);q.bad_observe=1;c.submitted_frames=91;assert(fvm_swf_sound_mix_queue_clock_info(q.owner,&c)==FVM_ERROR && c.submitted_frames==91);q.bad_observe=0;assert(adapted.pause(adapted.userdata,1)==FVM_OK && adapted.clear(adapted.userdata)==FVM_OK);assert(fvm_swf_sound_mix_queue_clock_info(q.owner,&c)==FVM_OK && !c.submitted_frames && !c.played_frames && !c.queued_frames);
 fvm_swf_sound_mix_queue_close(q.owner);fvm_swf_sound_mixer_close(mixer);return 0;
}
