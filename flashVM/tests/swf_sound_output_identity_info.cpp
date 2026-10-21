/* Native diagnostics distinguish time to a registered deadline from remaining voiced samples. */
#include "swf_sound_output_fixture.hpp"
using namespace output_fixture;
using namespace conversion_fixture;
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    Samples samples;samples.resize(7);Bytes swf=movie(samples,3,1);FVM_SWFSoundVoice *a=0,*b=0,*shorter=0,*longer=0;
    assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK && fvm_swf_sound_voice_open(c,0,&a)==FVM_OK && fvm_swf_sound_voice_open(c,0,&b)==FVM_OK);
    assert(fvm_swf_sound_voice_open(c,0,&shorter)==FVM_OK && fvm_swf_sound_voice_seek(shorter,0,6)==FVM_OK);
    swf=movie(samples,3,1,3);
    assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK && fvm_swf_sound_voice_open(c,0,&longer)==FVM_OK);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);
    FVM_SWFSoundMixer *mixer=create(48000,2,3);Device device;FVM_AudioQueueBackend table=backend(device);FVM_SWFSoundOutput *output=0;
    assert(fvm_swf_sound_output_create(mixer,&table,16,64,&output)==FVM_OK && fvm_swf_sound_output_set_identity_tracking(output,1)==FVM_OK);
    FVM_SWFSoundOutputIdentityInfo info;info.struct_size=sizeof(info);
    assert(fvm_swf_sound_output_submit_prepared_bounded(output,&a,(size_t)-1)==FVM_OK);
    size_t n=0;assert(fvm_swf_sound_output_pump(output,&n)==FVM_OK);
    assert(fvm_swf_sound_output_identity_info(output,7,&info)==FVM_OK && info.sound_id==7 && info.end_frame==8 && info.until_end_frames==8 && info.active && !info.played_frames);
    assert(fvm_swf_sound_output_pause(output,0)==FVM_OK);consume(device,3);
    assert(fvm_swf_sound_output_identity_info(output,7,&info)==FVM_OK && info.played_frames==3 && info.until_end_frames==5);
    consume(device,5);
    assert(fvm_swf_sound_output_identity_info(output,7,&info)==FVM_OK && info.end_frame==8 && !info.until_end_frames && !info.active);
    assert(fvm_swf_sound_output_submit_prepared_bounded(output,&b,(size_t)-1)==FVM_OK);
    assert(fvm_swf_sound_output_identity_info(output,7,&info)==FVM_OK && info.end_frame==24 && info.until_end_frames==16); /* Includes eight queued silence frames before the fresh eight-frame source. */
    assert(fvm_swf_sound_output_submit_prepared_bounded(output,&shorter,(size_t)-1)==FVM_OK);
    assert(fvm_swf_sound_output_identity_info(output,7,&info)==FVM_OK && info.end_frame==24); /* A shorter overlapping start must not shorten the old deadline. */
    assert(fvm_swf_sound_output_submit_prepared_bounded(output,&longer,(size_t)-1)==FVM_OK);
    assert(fvm_swf_sound_output_identity_info(output,7,&info)==FVM_OK && info.end_frame==39 && info.until_end_frames==31);
    info.end_frame=123;device.failure=2;
    assert(fvm_swf_sound_output_identity_info(output,7,&info)==FVM_ERROR && info.end_frame==123);device.failure=0;
    info.struct_size=0;assert(fvm_swf_sound_output_identity_info(output,7,&info)==FVM_INVALID_ARGUMENT && !info.struct_size && info.end_frame==123);info.struct_size=sizeof(info);
    assert(fvm_swf_sound_output_identity_info(output,0,&info)==FVM_INVALID_ARGUMENT && info.end_frame==123);
    assert(fvm_swf_sound_output_stop_id(output,7)==FVM_OK && fvm_swf_sound_output_identity_info(output,7,&info)==FVM_OK && !info.end_frame && !info.active);
    assert(fvm_swf_sound_output_stop(output)==FVM_OK && fvm_swf_sound_output_set_identity_tracking(output,0)==FVM_OK);
    assert(fvm_swf_sound_output_identity_info(output,7,&info)==FVM_UNSUPPORTED);
    fvm_swf_sound_output_close(output);fvm_swf_sound_mixer_close(mixer);return 0;
}
