/* Caller-buffered stream producer init/pump/seek do not allocate after original VM teardown. */
#define main allocation_reference_main
#include "native_instance_allocation.cpp"
#undef main
#define main stream_reference_main
#include "swf_stream_sound.cpp"
#undef main
#include "flashvm/swf_stream_output.h"
/* Model callbacks use only caller-owned scalar state and must not allocate. */
static FVM_Result queue_write(void *raw,const short *,size_t n) { *(size_t *)raw+=n;return FVM_OK; }
static FVM_Result queue_count(void *raw,size_t *n) { *n=*(size_t *)raw;return FVM_OK; }
static FVM_Result queue_pause(void *,int) { return FVM_OK; }
static FVM_Result queue_clear(void *raw) { *(size_t *)raw=0;return FVM_OK; }
/* Independent native source and all caller-buffered state survive VM/runtime destruction. */
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    Bytes tags,data;data.push_back(255);tag(tags,45,header(12));tag(tags,19,data);load(c,movie(tags));
    FVM_SWFStreamSoundSnapshot *snapshot=0;assert(fvm_swf_stream_sound_snapshot_open(c,0,&snapshot)==FVM_OK);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);allowance=0;
    FVM_SWFStreamConverter converter;assert(fvm_swf_stream_converter_init(&converter,snapshot,48000,1)==FVM_OK);
    size_t count=0,accepted=999;short pcm[2];FVM_AudioQueueBackend backend;std::memset(&backend,0,sizeof(backend));
    backend.struct_size=sizeof(backend);backend.abi_version=FVM_AUDIO_QUEUE_ABI_VERSION;backend.sample_rate=48000;backend.channels=1;
    backend.userdata=&count;backend.write=queue_write;backend.queued=queue_count;backend.pause=queue_pause;backend.clear=queue_clear;
    FVM_SWFStreamOutput output;assert(fvm_swf_stream_output_init(&output,&converter,&backend,pcm,2,2,2)==FVM_OK);
    unsigned frame=0;assert(fvm_swf_stream_output_next_frame(&output,&frame)==FVM_OK && frame==1);
    assert(fvm_swf_stream_output_pump_until(&output,0,&accepted)==FVM_NOT_FOUND && !accepted);
    assert(fvm_swf_stream_output_pump_until(&output,1,&accepted)==FVM_OK && accepted==2 && pcm[0]==32512 && pcm[1]==32512);
    assert(fvm_swf_stream_output_pause(&output,0)==FVM_OK);count=0;
    FVM_SWFStreamOutputInfo info;info.struct_size=sizeof(info);
    assert(fvm_swf_stream_output_info(&output,&info)==FVM_OK && info.played_frames==2);
    assert(fvm_swf_stream_output_pump(&output,&accepted)==FVM_NOT_FOUND && output.info.eof);
    assert(fvm_swf_stream_output_seek(&output,0,0)==FVM_OK && !output.info.eof && output.info.paused);
    assert(fvm_swf_stream_output_pump(&output,&accepted)==FVM_OK && accepted==2);
    fvm_swf_stream_sound_snapshot_close(snapshot);allowance=-1;
    return 0;
}
