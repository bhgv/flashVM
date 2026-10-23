/* Opt-in paired finite scheduling retains TAIL priority through callback failure and recursion. */
#include "swf_stream_mix_gate_fixture.hpp"
int main(int argc,char **argv)
{
 bool exceptions=argc==1;assert(argc==1||(argc==2&&!strcmp(argv[1],"--no-throw")));
 GateFixture f;f.fifo.reenter=1;FVM_SWFStreamConverter held=f.converter;size_t n=91;
 f.fifo.fail_count=1;assert(fvm_swf_stream_output_pump_mix_finite_until(&f.output,f.mix,0,&n)==FVM_ERROR && !n);
 f.fifo.fail_count=0;f.fifo.fail_write=2;
 assert(fvm_swf_stream_output_pump_mix_finite_until(&f.output,f.mix,0,&n)==FVM_NOT_FOUND && !n);
 if(exceptions){f.fifo.throw_write=1;assert(fvm_swf_stream_output_pump_mix_finite_until(&f.output,f.mix,1,&n)==FVM_ERROR && !n);f.fifo.throw_write=0;}
 assert(fvm_swf_sound_mix_queue_stop_all(f.mix)==FVM_OK);f.fifo.fail_write=0;
 assert(fvm_swf_stream_output_pump_mix_finite_until(&f.output,f.mix,1,&n)==FVM_OK && n==2 && !memcmp(&held,&f.converter,sizeof(held)));
 f.fifo.fail_clear=1;assert(fvm_swf_stream_output_seek(&f.output,0,0)==FVM_ERROR && f.output.info.submitted_frames==2);
 f.fifo.fail_clear=0;assert(fvm_swf_stream_output_seek(&f.output,0,0)==FVM_OK && !f.output.info.submitted_frames);
 return 0;
}
