/* Deterministic native FIFO around the actual C89 desktop owner, without a physical device. */
#ifndef FVM_TEST_DESKTOP_MIXED_NATIVE_HPP
#define FVM_TEST_DESKTOP_MIXED_NATIVE_HPP
#define FVM_DESKTOP_MIXED_FIXTURE_ONLY
#include "mixed_open.cpp"
#include "flashvm/execution_mode.h"
#include <cstdlib>
extern "C" FVM_Result fvm_test_mixed_refill(FVM_DesktopAudio *);
extern "C" FVM_Result fvm_test_mixed_install(FVM_Context *, FVM_DesktopAudio *);
struct MixedFIFO {size_t count,total,calls,observations;short samples[256];int fail_write,fail_observe,fail_clear,paused;};
static FVM_Result mixed_write(void *raw,const short *pcm,size_t frames)
{
 MixedFIFO *q=(MixedFIFO*)raw;++q->calls;if(q->fail_write)return FVM_ERROR;assert((q->total+frames)*2<=256);std::memcpy(q->samples+q->total*2,pcm,frames*2*sizeof(short));q->total+=frames;q->count+=frames;return FVM_OK;
}
static FVM_Result mixed_count(void *raw,size_t *out){MixedFIFO *q=(MixedFIFO*)raw;++q->observations;if(q->fail_observe)return q->fail_observe==2?FVM_UNSUPPORTED:FVM_ERROR;*out=q->count;return FVM_OK;}
static FVM_Result mixed_pause(void *raw,int paused){((MixedFIFO*)raw)->paused=paused;return FVM_OK;}
static FVM_Result mixed_clear(void *raw){MixedFIFO *q=(MixedFIFO*)raw;if(q->fail_clear)return FVM_ERROR;q->count=0;return FVM_OK;}
struct MixedNative
{
 FVM_Runtime *rt;FVM_Context *context;FVM_Object *clip;FVM_DesktopAudio audio;MixedFIFO fifo;
 MixedNative(FVM_ExecutionMode mode=FVM_EXECUTION_INTERPRETER,const Bytes *custom=0)
 {
  FVM_Config cfg;rt=0;context=0;clip=0;std::memset(&audio,0,sizeof(audio));std::memset(&fifo,0,sizeof(fifo));assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&context)==FVM_OK && fvm_standard_install(context)==FVM_OK);Bytes swf=custom?*custom:mixed_movie();assert(fvm_load_memory(context,swf.begin(),swf.size())==FVM_OK && fvm_movieclip_load_swf_ex(context,swf.begin(),swf.size(),"root",FVM_SWF_IMPORT_MUTE_ROOT_STREAMS,&clip)==FVM_OK && fvm_gc_pin(context,clip)==FVM_OK && fvm_execution_set_mode(context,mode,0)==FVM_OK);
  assert(fvm_swf_stream_sound_count(context,&audio.stream_count)==FVM_OK && audio.stream_count);audio.stream_segments=(FVM_DesktopStreamSegment*)std::calloc(audio.stream_count,sizeof(FVM_DesktopStreamSegment));assert(audio.stream_segments);for(size_t i=0;i<audio.stream_count;++i){FVM_SWFStreamSoundInfo info;info.struct_size=sizeof(info);assert(fvm_swf_stream_sound_at(context,i,&info)==FVM_OK);audio.stream_segments[i].frame=info.frame;assert(fvm_swf_stream_sound_snapshot_open_pcm(context,i,&audio.stream_segments[i].source)==FVM_OK && fvm_swf_stream_converter_init_continuous(&audio.stream_segments[i].converter,audio.stream_segments[i].source,48000,2)==FVM_OK);}audio.stream_source=audio.stream_segments[0].source;
  FVM_SWFSoundMixerConfig geometry={sizeof(geometry),48000,2,32,FVM_SWF_SOUND_MIXER_CONVERT|FVM_SWF_SOUND_MIXER_DECLICK};assert(fvm_swf_sound_mixer_create_bounded(&geometry,FVM_DESKTOP_AUDIO_EVENT_BYTES,&audio.mixer)==FVM_OK);FVM_AudioQueueBackend real,adapted;std::memset(&real,0,sizeof(real));real.struct_size=sizeof(real);real.abi_version=FVM_AUDIO_QUEUE_ABI_VERSION;real.sample_rate=48000;real.channels=2;real.userdata=&fifo;real.write=mixed_write;real.queued=mixed_count;real.pause=mixed_pause;real.clear=mixed_clear;size_t bytes=0;assert(fvm_swf_sound_mix_queue_storage_required_for_tracking(2,2,1,&bytes)==FVM_OK && fvm_swf_sound_mix_queue_create_bounded(audio.mixer,&real,2,bytes,&audio.composition,&adapted)==FVM_OK && fvm_swf_sound_mix_queue_clock_begin(audio.composition)==FVM_OK && fvm_swf_sound_mix_queue_set_identity_tracking(audio.composition,1,bytes)==FVM_OK);audio.target_frames=4;audio.resume_frames=2;audio.mutex=SDL_CreateMutex();assert(audio.mutex);assert(fvm_swf_stream_output_init(&audio.stream_output,&audio.stream_segments[0].converter,&adapted,audio.stream_pcm,sizeof(audio.stream_pcm)/sizeof(short),2,4)==FVM_OK && fvm_test_mixed_install(context,&audio)==FVM_OK && fvm_desktop_audio_timeline(context,clip,&audio)==FVM_OK);
 }
 ~MixedNative(){if(context)assert(fvm_movieclip_set_sound_callback(context,0,0)==FVM_OK);fvm_desktop_audio_close(&audio);if(context){fvm_gc_unpin(context,clip);fvm_context_destroy(context);fvm_runtime_destroy(rt);}}
 void advance(){assert(fvm_movieclip_advance(context,clip,1)==FVM_OK && fvm_desktop_audio_timeline(context,clip,&audio)==FVM_OK);}
 void refill(){assert(fvm_test_mixed_refill(&audio)==FVM_OK);}
};
#endif
