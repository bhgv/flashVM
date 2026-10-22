/* Global event cancellation keeps accepted PCM and clock while retiring every voice. */
#include "swf_sound_mix_identity_fixture.hpp"
int main()
{
 IdentityFixture f;FVM_SWFSoundVoice *voice=f.clone(0);
 assert(fvm_swf_sound_mix_queue_submit_prepared_bounded(f.queue,&voice,(size_t)-1)==FVM_OK);
 size_t n=0;assert(fvm_swf_sound_mix_queue_pump_tail(f.queue,2,8,&n)==FVM_OK);
 assert(fvm_swf_sound_mix_queue_stop_all(f.queue)==FVM_OK && !active(f.mixer));
 FVM_SWFSoundMixQueueIdentityInfo id;id.struct_size=sizeof(id);
 assert(fvm_swf_sound_mix_queue_identity_info(f.queue,7,&id)==FVM_OK && !id.end_frame && !id.active);
 FVM_SWFSoundMixQueueClock clock;clock.struct_size=sizeof(clock);
 assert(fvm_swf_sound_mix_queue_clock_info(f.queue,&clock)==FVM_OK && clock.submitted_frames==2 && clock.queued_frames==2 && !clock.played_frames);
 assert(f.fifo.total==2 && f.fifo.saved[0]==100 && f.fifo.saved[1]==101);
 voice=f.clone(1);assert(fvm_swf_sound_mix_queue_submit_prepared_bounded(f.queue,&voice,(size_t)-1)==FVM_OK && !voice);
 assert(fvm_swf_sound_mix_queue_identity_info(f.queue,7,&id)==FVM_OK && id.end_frame==7);
 assert(fvm_swf_sound_mix_queue_stop_all(f.queue)==FVM_OK && fvm_swf_sound_mix_queue_stop_all(f.queue)==FVM_OK);
 return 0;
}
