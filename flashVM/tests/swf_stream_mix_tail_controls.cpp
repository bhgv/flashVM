/* Paired tail validation, callback recursion and failed writes cannot split accepted epochs. */
#define FVM_MIX_IDENTITY_STREAM_FIXTURE_ONLY
#include "swf_sound_mix_identity_stream.cpp"
static FVM_SWFStreamOutput *reentrant_stream;
static FVM_SWFSoundMixQueue *reentrant_mix;
static FVM_Result paired_write(void *raw,const short *pcm,size_t frames)
{
 if(reentrant_stream){size_t n=91;assert(fvm_swf_stream_output_pump_mix_tail(reentrant_stream,reentrant_mix,&n)==FVM_INVALID_ARGUMENT && !n);FVM_SWFStreamOutputInfo out;out.struct_size=sizeof(out);out.submitted_frames=91;assert(fvm_swf_stream_output_info(reentrant_stream,&out)==FVM_INVALID_ARGUMENT && out.submitted_frames==91);}return write_pcm(raw,pcm,frames);
}
int main(int argc,char **argv)
{
 bool exceptions=argc==1;assert(argc==1||(argc==2&&!std::strcmp(argv[1],"--no-throw")));
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,movie,sizeof(movie))==FVM_OK);FVM_SWFStreamSoundSnapshot *source=0;assert(fvm_swf_stream_sound_snapshot_open_pcm(c,0,&source)==FVM_OK);FVM_SWFSoundVoice *voice[2]={0,0};for(unsigned i=0;i<2;++i)assert(fvm_swf_sound_voice_open(c,i,voice+i)==FVM_OK);fvm_context_destroy(c);fvm_runtime_destroy(rt);
 FVM_SWFSoundMixerConfig geometry={sizeof(geometry),48000,2,2,FVM_SWF_SOUND_MIXER_CONVERT};FVM_SWFSoundMixer *mixer=0;assert(fvm_swf_sound_mixer_create_configured(&geometry,&mixer)==FVM_OK);Queue q;std::memset(&q,0,sizeof(q));FVM_AudioQueueBackend b;std::memset(&b,0,sizeof(b));b.struct_size=sizeof(b);b.abi_version=FVM_AUDIO_QUEUE_ABI_VERSION;b.sample_rate=48000;b.channels=2;b.userdata=&q;b.write=paired_write;b.queued=queued;b.pause=pause_native;b.clear=clear_native;FVM_AudioQueueBackend adapter;FVM_SWFSoundMixQueue *mix=0;size_t bytes=0;assert(fvm_swf_sound_mix_queue_create(mixer,&b,2,&mix,&adapter)==FVM_OK);q.adapter=&adapter;assert(fvm_swf_sound_mix_queue_clock_begin(mix)==FVM_OK && fvm_swf_sound_mix_queue_storage_required_for_tracking(2,2,1,&bytes)==FVM_OK && fvm_swf_sound_mix_queue_set_identity_tracking(mix,1,bytes)==FVM_OK);for(unsigned i=0;i<2;++i)assert(fvm_swf_sound_mix_queue_submit_prepared_bounded(mix,voice+i,(size_t)-1)==FVM_OK);

 FVM_SWFStreamConverter converter;assert(fvm_swf_stream_converter_init(&converter,source,48000,2)==FVM_OK);FVM_SWFStreamOutput output;short retry[4];assert(fvm_swf_stream_output_init(&output,&converter,&adapter,retry,4,2,4)==FVM_OK);
 size_t n=91;assert(fvm_swf_stream_output_pump_mix_tail(0,mix,&n)==FVM_INVALID_ARGUMENT && !n);
 for(unsigned i=0;i<16;++i){q.count=0;FVM_Result r=fvm_swf_stream_output_pump(&output,&n);if(r==FVM_NOT_FOUND)break;assert(r==FVM_OK);}assert(q.accepted==9);FVM_SWFStreamOutput wrong=output;wrong.backend.userdata=&q;assert(fvm_swf_stream_output_pump_mix_tail(&wrong,mix,&n)==FVM_INVALID_ARGUMENT && !n);
 reentrant_stream=&output;reentrant_mix=mix;q.fail_observe=1;assert(fvm_swf_stream_output_pump_mix_tail(&output,mix,&n)==FVM_ERROR && !n);q.fail_observe=0;q.fail=1;assert(fvm_swf_stream_output_pump_mix_tail(&output,mix,&n)==FVM_ERROR && !n);if(exceptions){q.throwing=1;assert(fvm_swf_stream_output_pump_mix_tail(&output,mix,&n)==FVM_ERROR && !n);q.throwing=0;}q.fail=0;assert(fvm_swf_stream_output_pump_mix_tail(&output,mix,&n)==FVM_OK && n==2);FVM_SWFStreamOutputInfo info;info.struct_size=sizeof(info);assert(fvm_swf_stream_output_info(&output,&info)==FVM_OK && info.submitted_frames==11);
 reentrant_stream=0;q.count=0;assert(fvm_swf_sound_mix_queue_pump_tail(mix,2,4,&n)==FVM_OK);assert(fvm_swf_stream_output_pump_mix_tail(&output,mix,&n)==FVM_INVALID_ARGUMENT && !n);assert(fvm_swf_stream_output_seek(&output,0,0)==FVM_OK);fvm_swf_sound_mix_queue_close(mix);fvm_swf_sound_mixer_close(mixer);fvm_swf_stream_sound_snapshot_close(source);return 0;
}
