/* Partial tail cancellation, callback recursion and exceptions preserve atomic retry ownership. */
#include "swf_stream_mix_gate_fixture.hpp"
int main(int argc,char **argv)
{
 bool exceptions=argc==1;assert(argc==1||(argc==2&&!strcmp(argv[1],"--no-throw")));
 GateFixture f;size_t n=91;f.fifo.reenter=2;f.fifo.fail_count=1;
 assert(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix,2,1,&n)==FVM_ERROR && !n && !f.fifo.calls);
 f.fifo.fail_count=0;f.fifo.fail_write=2;
 assert(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix,2,1,&n)==FVM_NOT_FOUND && !n);
 if(exceptions){f.fifo.throw_write=1;assert(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix,2,4,&n)==FVM_ERROR && !n);f.fifo.throw_write=0;}
 assert(fvm_swf_sound_mix_queue_stop_all(f.mix)==FVM_OK);
 FVM_SWFSoundMixQueuePending p;p.struct_size=sizeof(p);
 assert(fvm_swf_sound_mix_queue_pending(f.mix,&p)==FVM_OK && p.frames==1);
 f.fifo.fail_clear=1;assert(f.adapted.clear(f.adapted.userdata)==FVM_ERROR);
 assert(fvm_swf_sound_mix_queue_pending(f.mix,&p)==FVM_OK && p.frames==1);
 f.fifo.fail_write=0;assert(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix,2,4,&n)==FVM_OK && n==1);
 assert(fvm_swf_sound_mix_queue_pump_tail_up_to(f.mix,2,4,&n)==FVM_NOT_FOUND && !n);
 f.fifo.fail_clear=0;assert(f.adapted.clear(f.adapted.userdata)==FVM_OK);
 FVM_SWFSoundMixQueueClock clock;clock.struct_size=sizeof(clock);
 assert(fvm_swf_sound_mix_queue_clock_info(f.mix,&clock)==FVM_OK && !clock.submitted_frames && !clock.played_frames);
 {GateFixture b;short base[4]={100,200,300,400};b.fifo.fail_write=1;assert(b.adapted.write(b.adapted.userdata,base,2)==FVM_ERROR);size_t calls=b.fifo.calls;assert(fvm_swf_sound_mix_queue_pump_tail_up_to(b.mix,2,4,&n)==FVM_INVALID_ARGUMENT && !n && b.fifo.calls==calls);}
 return 0;
}
