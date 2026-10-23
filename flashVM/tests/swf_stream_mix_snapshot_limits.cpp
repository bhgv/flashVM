/* Seed exact-clock/ring boundaries for the paired observation without huge PCM or long playback. */
#include "../src/swf/sound_mix_queue.cpp"
#include "swf_sound_mix_origin_fixture.hpp"
int main()
{
 GateFixture f;enable_origins(f);double seed=FVM_SWF_SOUND_OUTPUT_MAX_CLOCK-4;
 f.mix->clock_submitted=f.mix->clock_played=f.mix->origin_base_submitted=f.mix->origin_base_played=seed;
 f.output.info.submitted_frames=f.output.info.played_frames=seed;
 f.mix->origin_head=FVM_SWF_SOUND_MIX_QUEUE_ORIGIN_FRAMES-1;
 short pcm[4]={0,0,0,0};size_t n=0;
 assert(fvm_swf_sound_mix_queue_pump_tail_finite(f.mix,2,4,&n)==FVM_OK&&n==2);
 assert(f.adapted.write(f.adapted.userdata,pcm,2)==FVM_OK);
 f.output.info.submitted_frames=FVM_SWF_SOUND_OUTPUT_MAX_CLOCK;
 f.fifo.count=1;FVM_SWFStreamMixInfo info;info.struct_size=sizeof(info);size_t polls=f.fifo.count_calls;
 assert(fvm_swf_stream_output_mix_info(&f.output,f.mix,&info)==FVM_OK&&f.fifo.count_calls==polls+1);
 assert(info.stream.played_frames==seed+3&&info.stream.queued_frames==1);
 assert(info.origins.base_submitted_frames==seed+2&&info.origins.base_played_frames==seed+1);
 assert(info.origins.tail_submitted_frames==2&&info.origins.tail_played_frames==2);
 f.fifo.count=0;assert(fvm_swf_stream_output_mix_info(&f.output,f.mix,&info)==FVM_OK&&info.stream.played_frames==FVM_SWF_SOUND_OUTPUT_MAX_CLOCK);
 f.fifo.count=1;FVM_SWFStreamMixInfo before=info;
 assert(fvm_swf_stream_output_mix_info(&f.output,f.mix,&info)==FVM_ERROR&&!memcmp(&before,&info,sizeof(info)));
 f.fifo.count=0;assert(fvm_swf_stream_output_seek(&f.output,0,0)==FVM_OK);
 assert(fvm_swf_stream_output_mix_info(&f.output,f.mix,&info)==FVM_OK&&!info.stream.submitted_frames&&!info.origins.base_submitted_frames);
 {GateFixture g;enable_origins(g);short base[4]={1,1,1,1};assert(g.adapted.write(g.adapted.userdata,base,2)==FVM_OK);g.output.info.submitted_frames=2;g.output.target=1;FVM_SWFStreamOutputInfo cached=g.output.info;info=before;polls=g.fifo.count_calls;assert(fvm_swf_stream_output_mix_info(&g.output,g.mix,&info)==FVM_INVALID_ARGUMENT&&g.fifo.count_calls==polls+1&&!memcmp(&cached,&g.output.info,sizeof(cached))&&!memcmp(&before,&info,sizeof(info)));}
 return 0;
}
