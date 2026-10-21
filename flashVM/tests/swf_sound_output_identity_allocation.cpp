/* Deadline allocation is transactional; registered query/adoption/retry/stop paths allocate nothing. */
#define main allocation_reference_main
#include "native_instance_allocation.cpp"
#undef main
#include "swf_sound_output_fixture.hpp"
using namespace output_fixture;
using namespace conversion_fixture;
int main(int argc,char **argv)
{
    long begin=0,end=2;
    if(argc==3){assert(!std::strcmp(argv[1],"--allocation-budget"));char *tail=0;begin=std::strtol(argv[2],&tail,10);assert(tail && !*tail && begin>=0 && begin<=1);end=begin+1;}else assert(argc==1);
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    Samples samples;for(unsigned i=0;i<7;++i)samples.push_back(2000);
    Bytes swf=movie(samples,3,1);FVM_SWFSoundVoice *source=0,*control=0;
    assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK && fvm_swf_sound_voice_open(c,0,&source)==FVM_OK && fvm_swf_sound_voice_open(c,1,&control)==FVM_OK);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);
    for(long budget=begin;budget<end;++budget)
    {
        FVM_SWFSoundVoice *voice=0,*duplicate=0;
        assert(fvm_swf_sound_voice_clone(source,(size_t)-1,&voice)==FVM_OK && fvm_swf_sound_voice_clone(control,(size_t)-1,&duplicate)==FVM_OK);
        FVM_SWFSoundMixer *mixer=create(48000,2,1);Device device;FVM_AudioQueueBackend table=backend(device);FVM_SWFSoundOutput *output=0;
        assert(fvm_swf_sound_output_create(mixer,&table,16,64,&output)==FVM_OK && fvm_swf_sound_output_set_release(output,8)==FVM_OK);
        FVM_SWFSoundOutputStorageInfo before,after;before.struct_size=sizeof(before);after.struct_size=sizeof(after);
        assert(fvm_swf_sound_output_storage_info(output,&before)==FVM_OK);
        allowance=budget;FVM_Result result=fvm_swf_sound_output_set_identity_tracking(output,1);allowance=-1;
        if(!budget)
        {
            assert(result==FVM_OUT_OF_MEMORY && fvm_swf_sound_output_storage_info(output,&after)==FVM_OK && after.retained_bytes==before.retained_bytes && !after.identity_bytes);
            assert(fvm_swf_sound_output_set_identity_tracking(output,1)==FVM_OK);
        }
        else assert(result==FVM_OK);
        allowance=0;int playing=9;size_t n=0;
        assert(fvm_swf_sound_output_submit_prepared_bounded(output,&voice,(size_t)-1)==FVM_OK && !voice);
        device.failure=1;assert(fvm_swf_sound_output_pump(output,&n)==FVM_ERROR && !n && !active(mixer));
        assert(fvm_swf_sound_output_sound_active(output,7,&playing)==FVM_OK && playing);
        assert(fvm_swf_sound_output_submit_prepared_bounded(output,&duplicate,0)==FVM_OK && !duplicate);
        device.failure=0;assert(fvm_swf_sound_output_pump(output,&n)==FVM_OK && n==16);
        assert(fvm_swf_sound_output_stop_id(output,9)==FVM_OK);
        assert(fvm_swf_sound_output_sound_active(output,7,&playing)==FVM_OK && playing);
        device.failure=4;assert(fvm_swf_sound_output_stop_release(output)==FVM_ERROR);device.failure=0;
        assert(fvm_swf_sound_output_sound_active(output,7,&playing)==FVM_OK && playing);
        assert(fvm_swf_sound_output_stop_release(output)==FVM_OK && fvm_swf_sound_output_sound_active(output,7,&playing)==FVM_OK && !playing);
        assert(fvm_swf_sound_output_set_identity_tracking(output,0)==FVM_OK);
        assert(fvm_swf_sound_output_storage_info(output,&after)==FVM_OK && after.retained_bytes==before.retained_bytes);
        fvm_swf_sound_output_close(output);fvm_swf_sound_mixer_close(mixer);
        assert(allowance==0);allowance=-1;
    }
    fvm_swf_sound_voice_close(source);fvm_swf_sound_voice_close(control);return 0;
}
