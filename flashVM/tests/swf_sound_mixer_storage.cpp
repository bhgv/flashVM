/* Aggregate adoption is transactional and releases exact charges when native voices retire. */
#define main allocation_reference_main
#include "native_instance_allocation.cpp"
#undef main
#include "swf_sound_conversion_fixture.hpp"
using namespace conversion_fixture;
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    Samples samples;for(unsigned i=0;i<300;++i)samples.push_back(1200);
    Bytes swf=movie(samples,3,1);assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);
    FVM_SWFSoundVoice *a=0,*b=0,*duplicate=0;
    assert(fvm_swf_sound_voice_open(c,0,&a)==FVM_OK && fvm_swf_sound_voice_open(c,0,&b)==FVM_OK && fvm_swf_sound_voice_open(c,1,&duplicate)==FVM_OK);
    FVM_SWFSoundMixer *mixer=create(48000,2,2);
    FVM_SWFSoundMixerStorageInfo empty={sizeof(empty),0,0,0},used=empty;
    FVM_SWFSoundVoiceStorageInfo voice={sizeof(voice),0,0,0};
    assert(fvm_swf_sound_mixer_storage_info(mixer,&empty)==FVM_OK && !empty.voice_bytes && empty.fixed_bytes==empty.retained_bytes);
    FVM_SWFSoundMixerConfig geometry={sizeof(geometry),48000,2,2,FVM_SWF_SOUND_MIXER_CONVERT};
    FVM_SWFSoundMixer *other=(FVM_SWFSoundMixer*)1;
    assert(fvm_swf_sound_mixer_create_bounded(&geometry,empty.fixed_bytes-1,&other)==FVM_OUT_OF_MEMORY && !other);
    assert(fvm_swf_sound_mixer_create_bounded(&geometry,empty.fixed_bytes,&other)==FVM_OK && other);
    assert(fvm_swf_sound_mixer_storage_info(other,&used)==FVM_OK && used.fixed_bytes==empty.fixed_bytes);
    fvm_swf_sound_mixer_close(other);
    assert(fvm_swf_sound_voice_storage_info(a,&voice)==FVM_OK);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);allowance=0;
    size_t budget=empty.retained_bytes+voice.retained_bytes;
    int active=99;
    assert(fvm_swf_sound_mixer_sound_active(mixer,7,&active)==FVM_OK && !active);
    assert(fvm_swf_sound_mixer_sound_active(mixer,0,&active)==FVM_INVALID_ARGUMENT && !active);
    FVM_SWFSoundVoice *saved=a;
    assert(fvm_swf_sound_mixer_submit_prepared_bounded(mixer,&a,budget-1)==FVM_OUT_OF_MEMORY && a==saved);
    assert(fvm_swf_sound_mixer_submit_prepared_bounded(mixer,&a,budget)==FVM_OK && !a);
    assert(fvm_swf_sound_mixer_storage_info(mixer,&used)==FVM_OK && used.retained_bytes==budget && used.voice_bytes==voice.retained_bytes);
    assert(fvm_swf_sound_mixer_sound_active(mixer,7,&active)==FVM_OK && active);
    saved=b;assert(fvm_swf_sound_mixer_submit_prepared_bounded(mixer,&b,budget)==FVM_OUT_OF_MEMORY && b==saved);
    assert(fvm_swf_sound_mixer_submit_prepared_bounded(mixer,&duplicate,0)==FVM_OK && !duplicate);
    short pcm[1024];size_t n=0;
    assert(fvm_swf_sound_mixer_read(mixer,512,pcm,1024,&n)==FVM_OK && n==1024);
    assert(fvm_swf_sound_mixer_storage_info(mixer,&used)==FVM_OK && used.retained_bytes==empty.retained_bytes);
    assert(fvm_swf_sound_mixer_sound_active(mixer,7,&active)==FVM_OK && !active);
    assert(fvm_swf_sound_mixer_submit_prepared_bounded(mixer,&b,budget)==FVM_OK && !b);
    assert(fvm_swf_sound_mixer_stop_all(mixer)==FVM_OK);
    used.struct_size=0;
    assert(fvm_swf_sound_mixer_storage_info(mixer,&used)==FVM_INVALID_ARGUMENT && !used.struct_size);
    fvm_swf_sound_mixer_close(mixer);assert(allowance==0);allowance=-1;return 0;
}
