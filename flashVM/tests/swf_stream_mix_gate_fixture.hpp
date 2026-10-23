/* Native mixed source/voice snapshots after VM teardown with a deterministic observable FIFO. */
#ifndef FVM_TEST_STREAM_MIX_GATE_FIXTURE_HPP
#define FVM_TEST_STREAM_MIX_GATE_FIXTURE_HPP
#define main gated_tutorial_main
#include "../examples/multimedia/swf_stream_mix/main.c"
#undef main
#include "flashvm/swf_stream_output.h"
#include <cassert>
struct GateFIFO {size_t count,total,calls,drain_per_count;short samples[128];int fail_write,fail_count,fail_clear,throw_write,reenter;FVM_SWFStreamOutput *owner;FVM_SWFSoundMixQueue *mix;};
static void gate_reenter(GateFIFO *q){if(!q->reenter)return;if(q->reenter==2){size_t n=91;assert(fvm_swf_sound_mix_queue_pump_tail_up_to(q->mix,2,4,&n)==FVM_INVALID_ARGUMENT && !n);assert(fvm_swf_sound_mix_queue_stop_all(q->mix)==FVM_INVALID_ARGUMENT);return;}size_t n=91;assert(fvm_swf_stream_output_pump_mix_until(q->owner,q->mix,1,&n)==FVM_INVALID_ARGUMENT && !n);FVM_SWFStreamOutputInfo info;info.struct_size=sizeof(info);info.submitted_frames=91;assert(fvm_swf_stream_output_info(q->owner,&info)==FVM_INVALID_ARGUMENT && info.submitted_frames==91);}
static FVM_Result gate_write(void *raw,const short *pcm,size_t frames){GateFIFO *q=(GateFIFO*)raw;++q->calls;gate_reenter(q);if(q->throw_write)throw 7;if(q->fail_write)return q->fail_write==2?FVM_NOT_FOUND:FVM_ERROR;assert((q->total+frames)*2<=128);memcpy(q->samples+q->total*2,pcm,frames*2*sizeof(short));q->total+=frames;q->count+=frames;return FVM_OK;}
static FVM_Result gate_count(void *raw,size_t *out){GateFIFO *q=(GateFIFO*)raw;gate_reenter(q);if(q->fail_count)return FVM_ERROR;if(q->drain_per_count)q->count=q->count>q->drain_per_count?q->count-q->drain_per_count:0;*out=q->count;return FVM_OK;}
static FVM_Result gate_pause(void *,int){return FVM_OK;}
static FVM_Result gate_clear(void *raw){GateFIFO *q=(GateFIFO*)raw;if(q->fail_clear)return FVM_ERROR;q->count=0;return FVM_OK;}
struct GateFixture
{
 FVM_SWFStreamSoundSnapshot *source;FVM_SWFSoundMixer *mixer;FVM_SWFSoundMixQueue *mix;FVM_AudioQueueBackend adapted;FVM_SWFStreamConverter converter;FVM_SWFStreamOutput output;short retry[4];GateFIFO fifo;
 GateFixture(){source=0;mixer=0;mix=0;memset(&fifo,0,sizeof(fifo));FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,movie,sizeof(movie))==FVM_OK && fvm_swf_stream_sound_snapshot_open_pcm(c,0,&source)==FVM_OK);FVM_SWFSoundVoice *voices[2]={0,0};for(unsigned i=0;i<2;++i)assert(fvm_swf_sound_voice_open(c,i,voices+i)==FVM_OK);fvm_context_destroy(c);fvm_runtime_destroy(rt);FVM_SWFSoundMixerConfig geometry={sizeof(geometry),48000,2,2,FVM_SWF_SOUND_MIXER_CONVERT};assert(fvm_swf_sound_mixer_create_configured(&geometry,&mixer)==FVM_OK);FVM_AudioQueueBackend b;memset(&b,0,sizeof(b));b.struct_size=sizeof(b);b.abi_version=FVM_AUDIO_QUEUE_ABI_VERSION;b.sample_rate=48000;b.channels=2;b.userdata=&fifo;b.write=gate_write;b.queued=gate_count;b.pause=gate_pause;b.clear=gate_clear;size_t bytes=0;assert(fvm_swf_sound_mix_queue_storage_required_for_tracking(2,2,1,&bytes)==FVM_OK && fvm_swf_sound_mix_queue_create_bounded(mixer,&b,2,bytes,&mix,&adapted)==FVM_OK && fvm_swf_sound_mix_queue_clock_begin(mix)==FVM_OK && fvm_swf_sound_mix_queue_set_identity_tracking(mix,1,bytes)==FVM_OK);for(unsigned i=0;i<2;++i)assert(fvm_swf_sound_mix_queue_submit_prepared_bounded(mix,voices+i,(size_t)-1)==FVM_OK);assert(fvm_swf_stream_converter_init(&converter,source,48000,2)==FVM_OK && fvm_swf_stream_output_init(&output,&converter,&adapted,retry,4,2,4)==FVM_OK);fifo.owner=&output;fifo.mix=mix;}
 ~GateFixture(){fvm_swf_sound_mix_queue_close(mix);fvm_swf_sound_mixer_close(mixer);fvm_swf_stream_sound_snapshot_close(source);}
};
#endif
