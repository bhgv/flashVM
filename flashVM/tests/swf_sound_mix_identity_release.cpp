/* A cancelled ID lease permits a new NoMultiple voice while the old DECLICK tail releases. */
#include "swf_sound_mix_identity_fixture.hpp"
int main()
{
 IdentityFixture f(true,100,FVM_SWF_SOUND_MIXER_CONVERT|FVM_SWF_SOUND_MIXER_DECLICK);FVM_SWFSoundVoice *voice=f.clone(0);assert(fvm_swf_sound_mix_queue_submit_prepared_bounded(f.queue,&voice,(size_t)-1)==FVM_OK);size_t n=0;assert(fvm_swf_sound_mix_queue_pump_tail(f.queue,1,8,&n)==FVM_OK && f.fifo.saved[0]==100);assert(fvm_swf_sound_mix_queue_stop_id(f.queue,7)==FVM_OK && active(f.mixer)==1);voice=f.clone(1);assert(fvm_swf_sound_mix_queue_submit_prepared_bounded(f.queue,&voice,(size_t)-1)==FVM_OK && !voice && active(f.mixer)==2);FVM_SWFSoundMixQueueIdentityInfo info;info.struct_size=sizeof(info);assert(fvm_swf_sound_mix_queue_identity_info(f.queue,7,&info)==FVM_OK && info.end_frame==6);assert(fvm_swf_sound_mix_queue_pump_tail(f.queue,1,8,&n)==FVM_OK && f.fifo.saved[1]==201);return 0;
}
