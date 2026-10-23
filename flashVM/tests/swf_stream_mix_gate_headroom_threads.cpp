/* Four OS workers independently consume one FIFO frame per short-retry native scheduling step. */
#include "swf_stream_mix_gate_fixture.hpp"
#include "../ports/thread_port.h"
struct HeadroomJob {unsigned done;};
static void headroom_worker(void *raw)
{
 HeadroomJob *job=(HeadroomJob*)raw;
 for(unsigned epoch=0;epoch<64;++epoch)
 {
  GateFixture f;FVM_SWFStreamConverter held=f.converter;size_t n=0;
  for(unsigned i=0;i<2;++i)assert(fvm_swf_stream_output_pump_mix_until(&f.output,f.mix,0,&n)==FVM_OK && n==2);
  while(f.fifo.total<14)
  {
   assert(f.fifo.count==4);--f.fifo.count;f.fifo.fail_write=2;
   assert(fvm_swf_stream_output_pump_mix_until(&f.output,f.mix,0,&n)==FVM_NOT_FOUND && !n);
   f.fifo.fail_write=0;
   assert(fvm_swf_stream_output_pump_mix_until(&f.output,f.mix,1,&n)==FVM_OK && n==1);
   assert(!memcmp(&held,&f.converter,sizeof(held)));
  }
  for(unsigned frame=0;frame<14;++frame)for(unsigned channel=0;channel<2;++channel)
   assert(f.fifo.samples[frame*2+channel]==(frame<9?0:-30000));
  f.fifo.count=0;
  assert(fvm_swf_stream_output_pump_mix_until(&f.output,f.mix,0,&n)==FVM_NOT_FOUND && !n && f.fifo.total==14);
  FVM_SWFSoundMixQueueIdentityInfo id;id.struct_size=sizeof(id);
  assert(fvm_swf_sound_mix_queue_identity_info(f.mix,8,&id)==FVM_OK && !id.active && id.played_frames==14);
  assert(fvm_swf_stream_output_seek(&f.output,0,0)==FVM_OK);++job->done;
 }
}
int main()
{
 HeadroomJob jobs[4];FVM_PortThread *threads[4];
 for(unsigned i=0;i<4;++i){jobs[i].done=0;threads[i]=fvm_port_thread_start(headroom_worker,jobs+i);assert(threads[i]);}
 for(unsigned i=0;i<4;++i){fvm_port_thread_join(threads[i]);assert(jobs[i].done==64);}
 return 0;
}
