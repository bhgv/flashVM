/* A native voice remains NoMultiple-active through pending PCM and its exact queued endpoint. */
#include "swf_sound_output_fixture.hpp"
using namespace output_fixture;
using namespace conversion_fixture;
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    Samples samples;for(unsigned i=0;i<7;++i)samples.push_back((short)(1000+i*100));
    Bytes swf=movie(samples,3,1);assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);
    FVM_SWFSoundVoice *first=0,*duplicate=0,*next=0,*pending_duplicate=0,*last=0;
    assert(fvm_swf_sound_voice_open(c,0,&first)==FVM_OK && fvm_swf_sound_voice_open(c,1,&duplicate)==FVM_OK && fvm_swf_sound_voice_open(c,0,&next)==FVM_OK && fvm_swf_sound_voice_open(c,1,&pending_duplicate)==FVM_OK && fvm_swf_sound_voice_open(c,0,&last)==FVM_OK);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);
    FVM_SWFSoundMixer *mixer=create(48000,2,1);Device device;FVM_AudioQueueBackend table=backend(device);FVM_SWFSoundOutput *output=0;
    assert(fvm_swf_sound_output_create(mixer,&table,16,64,&output)==FVM_OK);device.output=output;
    FVM_SWFSoundVoice *saved=first;
    assert(fvm_swf_sound_output_submit_prepared_bounded(output,&first,(size_t)-1)==FVM_UNSUPPORTED && first==saved);
    assert(fvm_swf_sound_output_set_identity_tracking(output,1)==FVM_OK);
    assert(fvm_swf_sound_output_submit_prepared_bounded(output,&first,0)==FVM_OUT_OF_MEMORY && first==saved);
    int playing=9;assert(fvm_swf_sound_output_sound_active(output,7,&playing)==FVM_OK && !playing);
    assert(fvm_swf_sound_output_submit_prepared_bounded(output,&first,(size_t)-1)==FVM_OK && !first);
    size_t n=0;assert(fvm_swf_sound_output_pump(output,&n)==FVM_OK && n==16 && !active(mixer));
    assert(fvm_swf_sound_output_sound_active(output,7,&playing)==FVM_OK && playing);
    assert(fvm_swf_sound_output_submit_prepared_bounded(output,&duplicate,0)==FVM_OK && !duplicate && !active(mixer));
    assert(fvm_swf_sound_output_pause(output,0)==FVM_OK);consume(device,7);
    assert(fvm_swf_sound_output_sound_active(output,7,&playing)==FVM_OK && playing);
    consume(device,1);assert(fvm_swf_sound_output_sound_active(output,7,&playing)==FVM_OK && !playing); /* Eight-frame ceil endpoint, not full sixteen-frame block. */
    assert(fvm_swf_sound_output_submit_prepared_bounded(output,&next,(size_t)-1)==FVM_OK && !next);
    device.failure=1;assert(fvm_swf_sound_output_pump(output,&n)==FVM_ERROR && !n && !active(mixer));
    assert(fvm_swf_sound_output_sound_active(output,7,&playing)==FVM_OK && playing);
    assert(fvm_swf_sound_output_submit_prepared_bounded(output,&pending_duplicate,0)==FVM_OK && !pending_duplicate);
    FVM_SWFSoundOutputInfo pending=output_fixture::info(output);assert(pending.pending_frames==16 && pending.submitted_frames==16);
    device.failure=0;assert(fvm_swf_sound_output_pump(output,&n)==FVM_OK && n==16);
    assert(!std::memcmp(device.history,device.history+32,32*sizeof(short)));
    consume(device,15);assert(fvm_swf_sound_output_sound_active(output,7,&playing)==FVM_OK && playing);
    consume(device,1);assert(fvm_swf_sound_output_sound_active(output,7,&playing)==FVM_OK && !playing);
    device.failure=2;saved=last;
    assert(fvm_swf_sound_output_submit_prepared_bounded(output,&last,(size_t)-1)==FVM_ERROR && last==saved);device.failure=0;
    assert(fvm_swf_sound_output_submit_prepared_bounded(output,&last,(size_t)-1)==FVM_OK && !last);
    fvm_swf_sound_output_close(output);fvm_swf_sound_mixer_close(mixer);return 0;
}
