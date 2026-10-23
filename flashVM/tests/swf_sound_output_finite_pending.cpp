/* Pending observes native voice, retry and FIFO stages without producing or consuming PCM. */
#include "swf_sound_output_finite_fixture.hpp"
int main()
{
 FiniteOutput f;f.device.reenter=true;int pending=91;size_t n=0;
 assert(fvm_swf_sound_output_pending(0,&pending)==FVM_INVALID_ARGUMENT&&pending==91);
 assert(fvm_swf_sound_output_pending(f.output,&pending)==FVM_OK&&!pending);
 f.start();assert(fvm_swf_sound_output_pending(f.output,&pending)==FVM_OK&&pending&&!f.device.values);
 f.device.failure=1;assert(fvm_swf_sound_output_pump_finite(f.output,&n)==FVM_ERROR&&!n);
 assert(fvm_swf_sound_output_pending(f.output,&pending)==FVM_OK&&pending&&!f.device.values);
 f.device.failure=0;for(unsigned i=0;i<3;++i){f.device.queued=0;assert(fvm_swf_sound_output_pump_finite(f.output,&n)==FVM_OK);}
 assert(!conversion_fixture::active(f.mixer));assert(fvm_swf_sound_output_pending(f.output,&pending)==FVM_OK&&pending);
 f.device.failure=2;pending=91;assert(fvm_swf_sound_output_pending(f.output,&pending)==FVM_ERROR&&pending==91);
 f.device.failure=0;f.device.queued=0;assert(fvm_swf_sound_output_pending(f.output,&pending)==FVM_OK&&!pending);
 assert(output_fixture::info(f.output).played_frames==9);return 0;
}
