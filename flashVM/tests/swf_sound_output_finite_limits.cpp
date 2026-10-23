/* Seed exact epoch limits; finite short work is checked before rendering and never overflows. */
#include "../src/swf/sound_output.cpp"
#include "swf_sound_output_finite_fixture.hpp"
int main()
{
 FiniteOutput f;f.start();short pcm[16];size_t n=0;assert(fvm_swf_sound_mixer_read(f.mixer,8,pcm,16,&n)==FVM_OK);
 f.output->info.submitted_frames=f.output->info.played_frames=FVM_SWF_SOUND_OUTPUT_MAX_CLOCK-1;
 assert(fvm_swf_sound_output_pump_finite(f.output,&n)==FVM_OK&&n==1&&f.output->info.submitted_frames==FVM_SWF_SOUND_OUTPUT_MAX_CLOCK);
 f.device.queued=0;assert(fvm_swf_sound_output_pump_finite(f.output,&n)==FVM_NOT_FOUND&&!n);
 f.start();size_t remaining=0;assert(fvm_swf_sound_mixer_active_frames(f.mixer,64,&remaining)==FVM_OK&&remaining==9);
 assert(fvm_swf_sound_output_pump_finite(f.output,&n)==FVM_ERROR&&!n);
 assert(fvm_swf_sound_mixer_active_frames(f.mixer,64,&remaining)==FVM_OK&&remaining==9);
 assert(fvm_swf_sound_output_stop(f.output)==FVM_OK&&!f.output->info.submitted_frames);return 0;
}
