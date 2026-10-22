/* Four actual OS workers independently change gates around retained native composition retries. */
#include "swf_stream_mix_gate_fixture.hpp"
#include "../ports/thread_port.h"
struct GateJob {unsigned done;};
static void gate_worker(void *raw)
{
 GateJob *job=(GateJob*)raw;for(unsigned epoch=0;epoch<64;++epoch){GateFixture f;size_t n=0;f.fifo.fail_write=1;assert(fvm_swf_stream_output_pump_mix_until(&f.output,f.mix,0,&n)==FVM_ERROR);f.fifo.fail_write=0;assert(fvm_swf_stream_output_pump_mix_until(&f.output,f.mix,1,&n)==FVM_OK && n==2);assert(fvm_swf_stream_output_pump_mix_until(&f.output,f.mix,1,&n)==FVM_OK && n==2 && f.fifo.samples[4]==30000 && f.fifo.samples[6]==-25125);for(unsigned step=0;step<24;++step){f.fifo.count=0;FVM_Result r=fvm_swf_stream_output_pump_mix_until(&f.output,f.mix,(epoch+step)%2,&n);assert(r==FVM_OK||r==FVM_NOT_FOUND);}f.fifo.count=0;FVM_SWFStreamOutputInfo info;info.struct_size=sizeof(info);assert(fvm_swf_stream_output_info(&f.output,&info)==FVM_OK && info.eof && info.played_frames==info.submitted_frames && info.submitted_frames==(double)f.fifo.total);FVM_SWFSoundMixQueueIdentityInfo id;id.struct_size=sizeof(id);assert(fvm_swf_sound_mix_queue_identity_info(f.mix,8,&id)==FVM_OK && !id.active && id.end_frame==14);assert(fvm_swf_stream_output_seek(&f.output,0,0)==FVM_OK);++job->done;}
}
int main()
{
 GateJob jobs[4];FVM_PortThread *threads[4];for(unsigned i=0;i<4;++i){jobs[i].done=0;threads[i]=fvm_port_thread_start(gate_worker,jobs+i);assert(threads[i]);}for(unsigned i=0;i<4;++i){fvm_port_thread_join(threads[i]);assert(jobs[i].done==64);}return 0;
}
