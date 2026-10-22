/* Shared epochs survive contiguous source changes and reset only after acknowledged staged rebind clear. */
#define main allocation_fixture_main
#include "native_instance_allocation.cpp"
#undef main
#define main stream_fixture_main
#include "swf_stream_sound.cpp"
#undef main
#include "flashvm/swf_stream_output.h"
#include "flashvm/swf_sound_mix_queue.h"
struct SegmentQueue { size_t queued,writes,clears,pauses;int fail_write,fail_clear;FVM_SWFStreamOutput *output;FVM_SWFStreamConverter *target;short data[16];size_t used; };
static FVM_Result write_segment(void *raw,const short *pcm,size_t frames)
{ SegmentQueue *q=(SegmentQueue *)raw;++q->writes;if(q->fail_write)return FVM_ERROR;assert(q->used+frames*2<=16);std::memcpy(q->data+q->used,pcm,frames*2*sizeof(short));q->used+=frames*2;q->queued+=frames;return FVM_OK; }
static FVM_Result count_segment(void *raw,size_t *frames) { *frames=((SegmentQueue *)raw)->queued;return FVM_OK; }
static FVM_Result pause_segment(void *raw,int) { SegmentQueue *q=(SegmentQueue *)raw;++q->pauses;if(q->output){assert(fvm_swf_stream_output_continue(q->output,q->target)==FVM_INVALID_ARGUMENT);assert(fvm_swf_stream_output_bind_frame(q->output,q->target,3)==FVM_INVALID_ARGUMENT);}return FVM_OK; }
static FVM_Result clear_segment(void *raw) { SegmentQueue *q=(SegmentQueue *)raw;++q->clears;if(q->fail_clear)return FVM_ERROR;q->queued=0;return FVM_OK; }
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
 Bytes tags,data,empty;data.push_back(0);data.push_back(255);tag(tags,45,header(12));tag(tags,19,data);tag(tags,1,empty);
 tag(tags,45,header(0x3e));tag(tags,1,empty);tag(tags,45,header(0x3b));data.clear();word(data,1000);word(data,(unsigned short)-1000);tag(tags,19,data);
 load(c,stream_movie(tags,8,3));FVM_SWFStreamSoundSnapshot *snapshot[3]={0,0,0};FVM_SWFStreamConverter converter[3];
 for(unsigned i=0;i<3;++i){assert(fvm_swf_stream_sound_snapshot_open(c,i,&snapshot[i])==FVM_OK);assert(fvm_swf_stream_converter_init(&converter[i],snapshot[i],44100,2)==FVM_OK);}
 fvm_context_destroy(c);fvm_runtime_destroy(rt);
 SegmentQueue q;std::memset(&q,0,sizeof(q));FVM_AudioQueueBackend b;std::memset(&b,0,sizeof(b));b.struct_size=sizeof(b);b.abi_version=FVM_AUDIO_QUEUE_ABI_VERSION;b.sample_rate=44100;b.channels=2;b.userdata=&q;b.write=write_segment;b.queued=count_segment;b.pause=pause_segment;b.clear=clear_segment;
 FVM_SWFSoundMixer *mixer=0;assert(fvm_swf_sound_mixer_create(44100,2,1,&mixer)==FVM_OK);FVM_SWFSoundMixQueue *composition=0;FVM_AudioQueueBackend adapted;assert(fvm_swf_sound_mix_queue_create(mixer,&b,2,&composition,&adapted)==FVM_OK && fvm_swf_sound_mix_queue_clock_begin(composition)==FVM_OK);FVM_SWFSoundMixQueueClock clock;clock.struct_size=sizeof(clock);
 short pcm[4];FVM_SWFStreamOutput output;assert(fvm_swf_stream_output_init(&output,&converter[0],&adapted,pcm,4,2,8)==FVM_OK);q.output=&output;q.target=&converter[2];
 allowance=0;
 assert(fvm_swf_stream_output_continue(&output,&converter[1])==FVM_INVALID_ARGUMENT && output.converter==&converter[0]);
 size_t n=0;assert(fvm_swf_stream_output_pump_until(&output,1,&n)==FVM_OK && n==2 && q.queued==2);
 FVM_SWFStreamOutputInfo before=output.info;size_t pauses=q.pauses;
 assert(fvm_swf_stream_output_continue(&output,&converter[1])==FVM_OK && q.queued==2 && q.pauses==pauses && !q.clears && output.info.submitted_frames==before.submitted_frames);
 assert(fvm_swf_stream_output_continue(&output,&converter[2])==FVM_OK && q.queued==2 && q.pauses==pauses);
 FVM_SWFStreamConverter saved=converter[2];assert(fvm_swf_stream_output_pump_until(&output,2,&n)==FVM_NOT_FOUND && !n && !std::memcmp(&saved,&converter[2],sizeof(saved)));
 assert(fvm_swf_stream_output_pump_until(&output,3,&n)==FVM_OK && n==2 && q.queued==4 && output.info.submitted_frames==4);
 assert(fvm_swf_sound_mix_queue_clock_info(composition,&clock)==FVM_OK && clock.submitted_frames==4 && clock.queued_frames==4 && !clock.played_frames);
 const short expected[]={-32768,-32768,32512,32512,1000,-1000,1000,-1000};assert(q.used==8 && !std::memcmp(q.data,expected,sizeof(expected)));
 assert(fvm_swf_stream_output_pump(&output,0)==FVM_NOT_FOUND && output.info.eof && q.queued==4);
 FVM_SWFStreamConverter mismatch=converter[0];mismatch.output_rate=48000;before=output.info;
 assert(fvm_swf_stream_output_continue(&output,&mismatch)==FVM_INVALID_ARGUMENT && !std::memcmp(&before,&output.info,sizeof(before)));
 assert(fvm_swf_stream_output_continue(&output,&converter[2])==FVM_INVALID_ARGUMENT);
 assert(fvm_swf_stream_output_bind_frame(&output,&converter[0],1)==FVM_OK && output.converter==&converter[0] && !q.queued && !output.info.submitted_frames && output.info.paused);
 q.fail_write=1;assert(fvm_swf_stream_output_pump_until(&output,1,0)==FVM_ERROR && output.info.pending_frames==2);q.fail_write=0;
 short pending[4];std::memcpy(pending,pcm,sizeof(pcm));saved=converter[2];FVM_SWFStreamConverter old=converter[0];before=output.info;
 q.fail_clear=1;assert(fvm_swf_stream_output_bind_frame(&output,&converter[2],3)==FVM_ERROR && output.converter==&converter[0]);
 assert(!std::memcmp(&saved,&converter[2],sizeof(saved)) && !std::memcmp(&old,&converter[0],sizeof(old)) && !std::memcmp(pending,pcm,sizeof(pcm)) && output.info.pending_frames==before.pending_frames);
 FVM_SWFSoundMixQueuePending retry;retry.struct_size=sizeof(retry);assert(fvm_swf_sound_mix_queue_pending(composition,&retry)==FVM_OK && retry.origin==FVM_SWF_SOUND_MIX_QUEUE_PENDING_BASE && retry.frames==2);assert(fvm_swf_sound_mix_queue_clock_info(composition,&clock)==FVM_OK && !clock.submitted_frames);
 q.fail_clear=0;assert(fvm_swf_stream_output_bind_frame(&output,&converter[2],3)==FVM_OK && !output.info.pending_frames && output.converter==&converter[2]);
 assert(fvm_swf_sound_mix_queue_pending(composition,&retry)==FVM_OK && !retry.frames && fvm_swf_sound_mix_queue_clock_info(composition,&clock)==FVM_OK && !clock.submitted_frames);assert(fvm_swf_stream_output_pump_until(&output,3,&n)==FVM_OK && n==2 && fvm_swf_sound_mix_queue_clock_info(composition,&clock)==FVM_OK && clock.submitted_frames==2 && clock.queued_frames==2);
 unsigned frame=0;pauses=q.pauses;assert(fvm_swf_stream_output_bind_frame(&output,&converter[0],0)==FVM_INVALID_ARGUMENT && q.pauses==pauses);
 assert(fvm_swf_stream_output_bind_frame(&output,&converter[0],99)==FVM_OK && fvm_swf_stream_output_next_frame(&output,&frame)==FVM_NOT_FOUND);
 assert(fvm_swf_sound_mix_queue_clock_info(composition,&clock)==FVM_OK && !clock.submitted_frames && !clock.played_frames && fvm_swf_sound_mix_queue_clock_end(composition)==FVM_OK);
 allowance=-1;fvm_swf_sound_mix_queue_close(composition);fvm_swf_sound_mixer_close(mixer);for(unsigned i=0;i<3;++i)fvm_swf_stream_sound_snapshot_close(snapshot[i]);return 0;
}
