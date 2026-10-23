/* Finite pumps retain ordinary and anonymous stop-release retries across errors and recursion. */
#include "swf_sound_output_finite_fixture.hpp"
int main(int argc,char **argv)
{
 bool exceptions=argc==1;assert(argc==1||(argc==2&&!std::strcmp(argv[1],"--no-throw")));
 FiniteOutput f;f.device.reenter=true;assert(fvm_swf_sound_output_set_release(f.output,3)==FVM_OK);f.start();size_t n=91;
 f.device.failure=1;assert(fvm_swf_sound_output_pump_finite(f.output,&n)==FVM_ERROR&&!n);
 if(exceptions)for(int mode=5;mode<=6;++mode){f.device.failure=mode;assert(fvm_swf_sound_output_pump_finite(f.output,&n)==(mode==5?FVM_OUT_OF_MEMORY:FVM_ERROR)&&!n);}
 f.device.failure=0;assert(fvm_swf_sound_output_pump_finite(f.output,&n)==FVM_OK&&n==4);
 assert(fvm_swf_sound_output_pause(f.output,0)==FVM_OK);output_fixture::consume(f.device,2);
 f.device.failure=4;assert(fvm_swf_sound_output_stop_release(f.output)==FVM_ERROR&&conversion_fixture::active(f.mixer));
 f.device.failure=1;assert(fvm_swf_sound_output_stop_release(f.output)==FVM_ERROR&&!conversion_fixture::active(f.mixer));
 assert(output_fixture::info(f.output).pending_frames==3);int pending=0;assert(fvm_swf_sound_output_pending(f.output,&pending)==FVM_OK&&pending);
 f.device.failure=0;assert(fvm_swf_sound_output_pump_finite(f.output,&n)==FVM_OK&&n==3);
 assert(f.device.history[8]==f.device.history[2]&&f.device.history[12]==0);
 assert(fvm_swf_sound_output_pump_finite(f.output,&n)==FVM_NOT_FOUND&&!n);
 assert(fvm_swf_sound_output_stop(f.output)==FVM_OK);assert(fvm_swf_sound_output_pending(f.output,&pending)==FVM_OK&&!pending);return 0;
}
