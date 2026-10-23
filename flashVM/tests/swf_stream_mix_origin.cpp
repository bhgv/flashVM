/* Held source phase, real converted BASE and interleaved event TAIL produce distinct dequeue clocks. */
#include "swf_sound_mix_origin_fixture.hpp"
int main()
{
 GateFixture f;enable_origins(f);FVM_SWFStreamConverter held=f.converter;size_t n=0;
 assert(fvm_swf_stream_output_pump_mix_finite_until(&f.output,f.mix,0,&n)==FVM_OK&&n==2);
 assert(!memcmp(&held,&f.converter,sizeof(held)));
 assert(fvm_swf_stream_output_pump_mix_finite_until(&f.output,f.mix,1,&n)==FVM_OK&&n==2);
 f.fifo.count=0;FVM_SWFSoundMixQueueOriginInfo info=origins(f);assert(info.base_played_frames==2&&info.tail_played_frames==2);
 for(unsigned i=0;i<20;++i){f.fifo.count=0;FVM_Result r=fvm_swf_stream_output_pump_mix_finite_until(&f.output,f.mix,1,&n);if(r==FVM_NOT_FOUND)break;assert(r==FVM_OK);}
 f.fifo.count=0;info=origins(f);assert(info.base_submitted_frames==9&&info.base_played_frames==9&&info.tail_submitted_frames==5&&info.tail_played_frames==5&&f.fifo.total==14);
 {GateFixture g;enable_origins(g);assert(fvm_swf_stream_output_pump_mix_finite_until(&g.output,g.mix,0,&n)==FVM_OK&&n==2);assert(fvm_swf_stream_output_pump_mix_finite_until(&g.output,g.mix,0,&n)==FVM_OK&&n==2);g.fifo.drain_per_count=1;assert(fvm_swf_stream_output_pump_mix_finite_until(&g.output,g.mix,1,&n)==FVM_OK&&n==2);assert(g.output.info.submitted_frames==6&&g.output.info.played_frames==3&&g.output.info.queued_frames==3);g.fifo.drain_per_count=0;info=origins(g);assert(info.tail_played_frames==3&&!info.base_played_frames&&info.base_queued_frames==2&&info.tail_queued_frames==1);}
 return 0;
}
