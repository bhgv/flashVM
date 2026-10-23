/* Adaptive native tails use partial FIFO room without splitting exact rejected retries. */
#include "swf_stream_mix_gate_fixture.hpp"
int main()
{
 GateFixture f;size_t n=91;
 assert(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix,0,4,&n)==FVM_INVALID_ARGUMENT && !n);
 assert(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix,3,4,&n)==FVM_INVALID_ARGUMENT && !n);
 assert(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix,2,0,&n)==FVM_INVALID_ARGUMENT && !n);
 assert(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix,2,1,&n)==FVM_OK && n==1 && f.fifo.count==1);
 assert(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix,2,4,&n)==FVM_OK && n==2 && f.fifo.count==3);
 size_t calls=f.fifo.calls;
 assert(fvm_swf_sound_mix_queue_pump_tail(f.mix,2,4,&n)==FVM_NOT_FOUND && !n && f.fifo.calls==calls);
 f.fifo.fail_write=2;
 assert(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix,2,4,&n)==FVM_NOT_FOUND && !n && f.fifo.count==3);
 FVM_SWFSoundMixQueuePending p;p.struct_size=sizeof(p);
 assert(fvm_swf_sound_mix_queue_pending(f.mix,&p)==FVM_OK && p.frames==1 && p.origin==FVM_SWF_SOUND_MIX_QUEUE_PENDING_TAIL);
 f.fifo.fail_write=0;
 assert(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix,2,4,&n)==FVM_OK && n==1 && f.fifo.count==4);
 assert(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix,2,4,&n)==FVM_NOT_FOUND && !n);
 f.fifo.count=0;f.fifo.fail_write=1;
 assert(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix,2,4,&n)==FVM_ERROR && !n);
 calls=f.fifo.calls;f.fifo.fail_write=0;
 assert(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix,1,4,&n)==FVM_NOT_FOUND && !n && f.fifo.calls==calls);
 assert(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix,2,1,&n)==FVM_NOT_FOUND && !n && f.fifo.calls==calls);
 assert(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix,2,4,&n)==FVM_OK && n==2 && f.fifo.count==2);
 assert(f.fifo.total==6);for(size_t i=0;i<6*2;++i)assert(f.fifo.samples[i]==0);
 FVM_SWFSoundMixQueueClock c;c.struct_size=sizeof(c);
 assert(fvm_swf_sound_mix_queue_clock_info(f.mix,&c)==FVM_OK && c.submitted_frames==6 && c.played_frames==4);
 return 0;
}
