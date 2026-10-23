/* Four independent native owners verify exact finite retries, PCM and drained IDs after source teardown. */
#include "swf_sound_output_finite_fixture.hpp"
#include "../ports/thread_port.h"
struct OutputJob{unsigned done;};
static void finite_output_worker(void *raw)
{
 OutputJob *job=(OutputJob*)raw;conversion_fixture::Samples samples;for(unsigned i=0;i<8;++i)samples.push_back((short)(2000+i*17));
 for(unsigned epoch=0;epoch<64;++epoch){FiniteOutput f(8,true);f.start(0,true);size_t n=0;for(unsigned step=0;step<3;++step){f.device.queued=0;f.device.failure=1;assert(fvm_swf_sound_output_pump_finite(f.output,&n)==FVM_ERROR&&!n);f.device.failure=0;assert(fvm_swf_sound_output_pump_finite(f.output,&n)==FVM_OK&&n==(step<2?4:1));}assert(f.device.values==18);for(unsigned frame=0;frame<9;++frame)for(unsigned ch=0;ch<2;++ch)assert(f.device.history[frame*2+ch]==conversion_fixture::oracle(samples,44100,1,48000,2,frame,ch));f.device.queued=0;int pending=91;assert(fvm_swf_sound_output_pending(f.output,&pending)==FVM_OK&&!pending);FVM_SWFSoundOutputIdentityInfo id;id.struct_size=sizeof(id);assert(fvm_swf_sound_output_identity_info(f.output,7,&id)==FVM_OK&&!id.active&&id.end_frame==9&&id.played_frames==9);++job->done;}
}
int main()
{
 OutputJob jobs[4];FVM_PortThread *threads[4];for(unsigned i=0;i<4;++i){jobs[i].done=0;threads[i]=fvm_port_thread_start(finite_output_worker,jobs+i);assert(threads[i]);}for(unsigned i=0;i<4;++i){fvm_port_thread_join(threads[i]);assert(jobs[i].done==64);}return 0;
}
