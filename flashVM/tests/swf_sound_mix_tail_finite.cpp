/* Finite tails trim fresh final work, but preserve retries created by legacy padded rendering. */
#include "swf_stream_mix_gate_fixture.hpp"
int main()
{
 {GateFixture f;size_t n=0;assert(fvm_swf_sound_mix_queue_pump_tail_finite(f.mix,1,4,&n)==FVM_OK && n==1);while(f.fifo.total<14){f.fifo.count=0;assert(fvm_swf_sound_mix_queue_pump_tail_finite(f.mix,2,4,&n)==FVM_OK && n && n<3);}assert(f.fifo.total==14 && n==1);f.fifo.count=0;assert(fvm_swf_sound_mix_queue_pump_tail_finite(f.mix,2,4,&n)==FVM_NOT_FOUND && !n);for(unsigned i=0;i<28;++i)assert(f.fifo.samples[i]==(i<18?0:-30000));}
 {GateFixture f;size_t n=0;for(unsigned i=0;i<13;++i){f.fifo.count=0;assert(fvm_swf_sound_mix_queue_pump_tail_finite(f.mix,1,4,&n)==FVM_OK && n==1);}f.fifo.count=0;f.fifo.fail_write=1;assert(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix,2,4,&n)==FVM_ERROR && !n);assert(fvm_swf_sound_mix_queue_stop_all(f.mix)==FVM_OK);f.fifo.fail_write=0;assert(fvm_swf_sound_mix_queue_pump_tail_finite(f.mix,2,4,&n)==FVM_OK && n==2 && f.fifo.total==15);assert(f.fifo.samples[26]==-30000 && f.fifo.samples[28]==0);}
 return 0;
}
