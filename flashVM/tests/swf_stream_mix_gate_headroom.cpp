/* A gated producer fills single-frame room and retries it before newly permitted base. */
#include "swf_stream_mix_gate_fixture.hpp"
int main()
{
 GateFixture f;FVM_SWFStreamConverter held=f.converter;size_t n=0;
 assert(fvm_swf_stream_output_pump_mix_until(&f.output,f.mix,0,&n)==FVM_OK && n==2);
 assert(fvm_swf_stream_output_pump_mix_until(&f.output,f.mix,0,&n)==FVM_OK && n==2);
 f.fifo.count=3;f.fifo.fail_write=1;
 assert(fvm_swf_stream_output_pump_mix_until(&f.output,f.mix,0,&n)==FVM_ERROR && !n);
 FVM_SWFSoundMixQueuePending p;p.struct_size=sizeof(p);
 assert(fvm_swf_sound_mix_queue_pending(f.mix,&p)==FVM_OK && p.frames==1 && p.origin==FVM_SWF_SOUND_MIX_QUEUE_PENDING_TAIL);
 f.fifo.count=2;f.fifo.fail_write=0;
 assert(fvm_swf_stream_output_pump_mix_until(&f.output,f.mix,1,&n)==FVM_OK && n==1);
 assert(!memcmp(&held,&f.converter,sizeof(held)) && f.output.info.submitted_frames==5);
 assert(fvm_swf_stream_output_pump_mix_until(&f.output,f.mix,1,&n)==FVM_OK && n==1);
 assert(memcmp(&held,&f.converter,sizeof(held)) && f.output.info.submitted_frames==6);
 assert(f.fifo.samples[10]==30000 && f.fifo.samples[11]==30000);
 FVM_SWFStreamOutputInfo info;info.struct_size=sizeof(info);
 assert(fvm_swf_stream_output_info(&f.output,&info)==FVM_OK && info.played_frames==2 && info.queued_frames==4);
 return 0;
}
