/* Seed exact clock boundaries; a one-frame request succeeds where a two-frame write cannot. */
#include "../src/swf/sound_mix_queue.cpp"
#include "swf_stream_mix_gate_fixture.hpp"
int main()
{
 GateFixture f;size_t n=91;
 f.mix->clock_submitted=f.mix->clock_played=FVM_SWF_SOUND_OUTPUT_MAX_CLOCK-1;
 assert(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix,2,1,&n)==FVM_OK && n==1);
 assert(f.mix->clock_submitted==FVM_SWF_SOUND_OUTPUT_MAX_CLOCK && f.fifo.total==1);
 f.fifo.count=0;size_t calls=f.fifo.calls;
 assert(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix,2,1,&n)==FVM_UNSUPPORTED && !n && f.fifo.calls==calls);
 assert(!f.mix->pending);
 assert(f.adapted.clear(f.adapted.userdata)==FVM_OK && !f.mix->clock_submitted);
 assert(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix,2,1,&n)==FVM_NOT_FOUND && !n);
 {GateFixture g;g.fifo.count=1;assert(fvm_swf_sound_mix_queue_pump_tail_up_to(g.mix,2,1,&n)==FVM_ERROR && !n && !g.fifo.calls);}
 {GateFixture g;assert(fvm_swf_sound_mix_queue_pump_tail_up_to(g.mix,2,FVM_SWF_SOUND_OUTPUT_MAX_QUEUE+1UL,&n)==FVM_INVALID_ARGUMENT && !n);}
 return 0;
}
