/* Standalone C89 contract for queued sound IDs, maximum UI16 identity and callback reentry. */
#include "flashvm/swf_sound_output.h"
#include <assert.h>
static const unsigned char movie[] = {
    70,87,83,8,84,0,0,0,8,0,0,12,1,0,149,3,
    7,0,62,7,0,0,0,232,3,76,4,176,4,20,5,120,
    5,220,5,64,6,149,3,255,255,62,7,0,0,0,232,3,
    76,4,176,4,20,5,120,5,220,5,64,6,195,3,7,0,
    0,195,3,7,0,16,195,3,255,255,0,195,3,255,255,16,
    64,0,0,0
};
static size_t queued_frames;
static int reenter;
static FVM_SWFSoundOutput *current;
static FVM_SWFSoundVoice **candidate;
static FVM_Result write_pcm(void *raw,const short *pcm,size_t frames)
{
    (void)raw;assert(pcm || !frames);queued_frames+=frames;return FVM_OK;
}
static FVM_Result queued(void *raw,size_t *out)
{
    (void)raw;
    if(reenter && current)
    {
        int active=99;
        FVM_SWFSoundOutputStorageInfo storage;
        FVM_SWFSoundOutputIdentityInfo identity;
        storage.struct_size=sizeof(storage);storage.fixed_bytes=123;
        identity.struct_size=sizeof(identity);identity.end_frame=123;
        assert(fvm_swf_sound_output_identity_info(current,7,&identity)==FVM_INVALID_ARGUMENT && identity.end_frame==123);
        assert(fvm_swf_sound_output_sound_active(current,7,&active)==FVM_INVALID_ARGUMENT && active==99);
        assert(fvm_swf_sound_output_stop_id(current,7)==FVM_INVALID_ARGUMENT);
        assert(fvm_swf_sound_output_set_identity_tracking(current,1)==FVM_INVALID_ARGUMENT);
        assert(fvm_swf_sound_output_storage_info(current,&storage)==FVM_INVALID_ARGUMENT && storage.fixed_bytes==123);
        if(candidate && *candidate)
        {
            FVM_SWFSoundVoice *saved=*candidate;
            assert(fvm_swf_sound_output_submit_prepared_bounded(current,candidate,(size_t)-1)==FVM_INVALID_ARGUMENT && *candidate==saved);
        }
    }
    *out=queued_frames;return FVM_OK;
}
static FVM_Result pause_queue(void *raw,int paused){(void)raw;(void)paused;return FVM_OK;}
static FVM_Result clear_queue(void *raw){(void)raw;queued_frames=0;return FVM_OK;}
int main(void)
{
    FVM_Config config;
    FVM_Runtime *runtime=0;
    FVM_Context *context=0;
    FVM_SWFSoundVoice *voices[4]={0,0,0,0};
    FVM_SWFSoundMixer *mixer=0;
    FVM_SWFSoundMixerConfig geometry={sizeof(geometry),48000,2,2,FVM_SWF_SOUND_MIXER_CONVERT};
    FVM_AudioQueueBackend backend={sizeof(backend),FVM_AUDIO_QUEUE_ABI_VERSION,48000,2,0,write_pcm,queued,pause_queue,clear_queue};
    FVM_SWFSoundOutputStorageInfo storage;
    size_t i,n=0;
    int active=99;
    double duration=-1;
    assert(fvm_config_init(&config)==FVM_OK && fvm_runtime_create(&config,&runtime)==FVM_OK && fvm_context_create(runtime,&context)==FVM_OK && fvm_load_memory(context,movie,sizeof(movie))==FVM_OK);
    for(i=0;i<4;++i)assert(fvm_swf_sound_voice_open(context,i,&voices[i])==FVM_OK);
    fvm_context_destroy(context);fvm_runtime_destroy(runtime);
    assert(fvm_swf_sound_voice_output_frames(voices[0],48000,&duration)==FVM_OK && duration==8);
    assert(fvm_swf_sound_mixer_create_configured(&geometry,&mixer)==FVM_OK && fvm_swf_sound_output_create(mixer,&backend,16,64,&current)==FVM_OK);
    assert(fvm_swf_sound_output_set_identity_tracking(current,1)==FVM_OK);reenter=1;
    for(i=0;i<4;i+=2)
    {
        candidate=&voices[i];
        assert(fvm_swf_sound_output_submit_prepared_bounded(current,&voices[i],(size_t)-1)==FVM_OK && !voices[i]);
    }
    assert(fvm_swf_sound_output_pump(current,&n)==FVM_OK && n==16);
    for(i=1;i<4;i+=2)
    {
        candidate=&voices[i];
        assert(fvm_swf_sound_output_submit_prepared_bounded(current,&voices[i],0)==FVM_OK && !voices[i]);
    }
    assert(fvm_swf_sound_output_sound_active(current,7,&active)==FVM_OK && active);
    assert(fvm_swf_sound_output_sound_active(current,65535,&active)==FVM_OK && active);
    storage.struct_size=sizeof(storage);
    assert(fvm_swf_sound_output_storage_info(current,&storage)==FVM_OK && storage.identity_bytes==FVM_SWF_SOUND_OUTPUT_ID_SLOTS*sizeof(double));
    assert(fvm_swf_sound_output_stop_id(current,65535)==FVM_OK);
    assert(fvm_swf_sound_output_sound_active(current,65535,&active)==FVM_OK && !active);
    assert(fvm_swf_sound_output_sound_active(current,7,&active)==FVM_OK && active);
    assert(fvm_swf_sound_output_stop(current)==FVM_OK);
    assert(fvm_swf_sound_output_sound_active(current,7,&active)==FVM_OK && !active);
    assert(fvm_swf_sound_output_set_identity_tracking(current,0)==FVM_OK);
    fvm_swf_sound_output_close(current);current=0;fvm_swf_sound_mixer_close(mixer);return 0;
}
