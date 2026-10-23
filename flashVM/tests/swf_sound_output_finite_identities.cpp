/* NoMultiple survives mixer retirement until the exact finite FIFO interval dequeues. */
#include "swf_sound_output_finite_fixture.hpp"
int main()
{
 FiniteOutput f(8,true);f.start(0,true);size_t n=0;int active=0;
 for(unsigned i=0;i<3;++i){f.device.queued=0;assert(fvm_swf_sound_output_pump_finite(f.output,&n)==FVM_OK);}
 assert(!conversion_fixture::active(f.mixer)&&f.device.values==18);
 assert(fvm_swf_sound_output_sound_active(f.output,7,&active)==FVM_OK&&active);
 f.start(1,true);assert(!conversion_fixture::active(f.mixer));
 f.device.queued=0;assert(fvm_swf_sound_output_sound_active(f.output,7,&active)==FVM_OK&&!active);
 f.start(1,true);assert(conversion_fixture::active(f.mixer)==1);
 for(unsigned i=0;i<3;++i){f.device.queued=0;assert(fvm_swf_sound_output_pump_finite(f.output,&n)==FVM_OK);}
 assert(f.device.values==36);f.device.queued=0;FVM_SWFSoundOutputIdentityInfo id;id.struct_size=sizeof(id);
 assert(fvm_swf_sound_output_identity_info(f.output,7,&id)==FVM_OK&&id.end_frame==18&&id.played_frames==18&&!id.active);
 return 0;
}
