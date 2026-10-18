/* Accepted-history release, paused/empty behavior, failure preservation and single-block retry. */
#include "swf_sound_output_fixture.hpp"
using namespace output_fixture;
int main()
{
    FVM_Config cfg;FVM_Runtime *runtime=0;FVM_Context *context=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&runtime)==FVM_OK);
    assert(fvm_context_create(runtime,&context)==FVM_OK);
    conversion_fixture::Samples samples;
    for(unsigned i=0;i<5000;++i)samples.push_back((short)(10000+i*2));
    conversion_fixture::Bytes swf=conversion_fixture::movie(samples,3,1);
    assert(fvm_load_memory(context,swf.begin(),swf.size())==FVM_OK);
    FVM_SWFSoundMixer *mixer=conversion_fixture::create(48000,2);
    Device device;FVM_AudioQueueBackend table=backend(device);FVM_SWFSoundOutput *output=0;
    assert(fvm_swf_sound_output_create(mixer,&table,8,16,&output)==FVM_OK);device.output=output;device.reenter=true;
    assert(fvm_swf_sound_output_stop_release(output)==FVM_INVALID_ARGUMENT);
    assert(fvm_swf_sound_output_set_release(output,1)==FVM_INVALID_ARGUMENT);
    assert(fvm_swf_sound_output_set_release(output,9)==FVM_INVALID_ARGUMENT);
    assert(fvm_swf_sound_output_set_release(output,4)==FVM_OK);
    assert(fvm_swf_sound_output_stop_release(output)==FVM_OK && device.paused && !device.queued);
    for(unsigned phase=0;phase<3;++phase) {
        assert(fvm_swf_sound_mixer_submit(mixer,context,0)==FVM_OK);
        assert(fvm_swf_sound_output_pump(output,0)==FVM_OK);
        assert(fvm_swf_sound_output_pump(output,0)==FVM_OK);
        assert(fvm_swf_sound_output_set_release(output,0)==FVM_INVALID_ARGUMENT);
        assert(fvm_swf_sound_output_pause(output,0)==FVM_OK);
        // Wrap the bounded ring while checking against an independent accepted-byte history.
        for(unsigned i=0;i<12;++i) {
            consume(device,8);
            assert(fvm_swf_sound_output_pump(output,0)==FVM_OK);
        }
        consume(device,8);
        short seed=device.history[device.values-device.queued*2-2];
        size_t before=device.values;
        device.failure=4;
        assert(fvm_swf_sound_output_stop_release(output)==FVM_ERROR);
        assert(conversion_fixture::active(mixer)==1 && device.values==before);
        device.failure=phase==1?1:0;
        FVM_Result result=fvm_swf_sound_output_stop_release(output);
        assert(result==(phase==1?FVM_ERROR:FVM_OK));
        assert(!device.paused && conversion_fixture::active(mixer)==0);
        if(phase==1) {
            assert(info(output).pending_frames==4 && device.values==before && !device.queued);
            device.failure=0;size_t accepted=0;
            assert(fvm_swf_sound_output_pump(output,&accepted)==FVM_OK && accepted==4);
        }
        assert(device.values==before+8 && device.queued==4);
        for(unsigned i=0;i<4;++i)
            for(unsigned c=0;c<2;++c)
                assert(device.history[before+i*2+c]==(short)((double)seed*(3-i)/3));
        assert(info(output).submitted_frames==4 && !info(output).pending_frames);
        size_t saved=device.values;
        assert(fvm_swf_sound_output_stop(output)==FVM_OK && !device.queued && device.paused);
        assert(device.values==saved); // Original immediate stop never synthesizes a tail.
    }
    assert(fvm_swf_sound_output_set_release(output,0)==FVM_OK);
    assert(fvm_swf_sound_output_stop_release(output)==FVM_INVALID_ARGUMENT);
    fvm_swf_sound_output_close(output);fvm_swf_sound_mixer_close(mixer);
    fvm_context_destroy(context);fvm_runtime_destroy(runtime);
}
