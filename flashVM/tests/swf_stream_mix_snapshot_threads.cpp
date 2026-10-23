/* Four independent OS workers verify coherent paired snapshots and finite tail-to-base handoff and exact final PCM after VM teardown. */
#include "swf_sound_mix_origin_fixture.hpp"
#include "../ports/thread_port.h"
struct FiniteJob {unsigned done;};
static void finite_worker(void *raw)
{
 FiniteJob *job=(FiniteJob*)raw;
 for(unsigned epoch=0;epoch<64;++epoch)
 {
  GateFixture f;enable_origins(f);FVM_SWFStreamConverter held=f.converter;size_t n=0;f.fifo.fail_write=2;
  assert(fvm_swf_stream_output_pump_mix_finite_until(&f.output,f.mix,0,&n)==FVM_NOT_FOUND && !n);
  f.fifo.fail_write=0;
  assert(fvm_swf_stream_output_pump_mix_finite_until(&f.output,f.mix,1,&n)==FVM_OK && n==2 && !memcmp(&held,&f.converter,sizeof(held)));
  for(unsigned i=0;i<24;++i){f.fifo.count=0;FVM_Result r=fvm_swf_stream_output_pump_mix_finite_until(&f.output,f.mix,1,&n);if(r==FVM_NOT_FOUND)break;assert(r==FVM_OK && n);}
  assert(f.fifo.total==14 && f.output.info.eof);
  FVM_SWFStreamMixInfo snapshot;snapshot.struct_size=sizeof(snapshot);size_t polls=f.fifo.count_calls;assert(fvm_swf_stream_output_mix_info(&f.output,f.mix,&snapshot)==FVM_OK&&f.fifo.count_calls==polls+1);assert(snapshot.stream.submitted_frames==14&&snapshot.stream.played_frames==14&&!snapshot.stream.queued_frames);FVM_SWFSoundMixQueueOriginInfo clock=snapshot.origins;assert(clock.base_submitted_frames==9&&clock.base_played_frames==9&&clock.tail_submitted_frames==5&&clock.tail_played_frames==5&&!clock.base_queued_frames&&!clock.tail_queued_frames);
  const long base[]={30000,-25125,11875,-10250,250,-1875,10500,4125,-30000};
  for(unsigned frame=0;frame<14;++frame){long expected=(frame>=2&&frame<11?base[frame-2]:0)+(frame<9?0:-30000);if(expected< -32768)expected= -32768;for(unsigned ch=0;ch<2;++ch)assert(f.fifo.samples[frame*2+ch]==expected);}
  f.fifo.count=0;FVM_SWFSoundMixQueueIdentityInfo id;id.struct_size=sizeof(id);
  assert(fvm_swf_sound_mix_queue_identity_info(f.mix,8,&id)==FVM_OK && !id.active && id.played_frames==14);
  assert(fvm_swf_stream_output_seek(&f.output,0,0)==FVM_OK);++job->done;
 }
}
int main()
{
 FiniteJob jobs[4];FVM_PortThread *threads[4];
 for(unsigned i=0;i<4;++i){jobs[i].done=0;threads[i]=fvm_port_thread_start(finite_worker,jobs+i);assert(threads[i]);}
 for(unsigned i=0;i<4;++i){fvm_port_thread_join(threads[i]);assert(jobs[i].done==64);}
 return 0;
}
