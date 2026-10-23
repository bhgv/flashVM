/* A single callback gives common/category clocks despite FIFO drain on every query. */
#include "swf_sound_mix_origin_fixture.hpp"
int main()
{
 GateFixture f;enable_origins(f);size_t n=0;
 assert(fvm_swf_stream_output_pump_mix_finite_until(&f.output,f.mix,0,&n)==FVM_OK&&n==2);
 assert(fvm_swf_stream_output_pump_mix_finite_until(&f.output,f.mix,1,&n)==FVM_OK&&n==2);
 FVM_SWFStreamConverter cursor=f.converter;short pcm[4];memcpy(pcm,f.retry,sizeof(pcm));
 f.fifo.drain_per_count=1;FVM_SWFStreamMixInfo info;memset(&info,0,sizeof(info));info.struct_size=sizeof(info);
 assert(fvm_swf_stream_output_mix_info(&f.output,f.mix,&info)==FVM_OK);
 assert(f.fifo.count==3&&info.stream.queued_frames==3&&info.stream.played_frames==1);
 assert(info.origins.tail_played_frames==1&&!info.origins.base_played_frames);
 assert(info.origins.base_queued_frames==2&&info.origins.tail_queued_frames==1);
 assert(info.origins.base_submitted_frames+info.origins.tail_submitted_frames==info.stream.submitted_frames);
 assert(!memcmp(&cursor,&f.converter,sizeof(cursor))&&!memcmp(pcm,f.retry,sizeof(pcm)));
 assert(info.stream.struct_size==sizeof(info.stream)&&info.origins.struct_size==sizeof(info.origins));
 assert(fvm_swf_stream_output_mix_info(&f.output,f.mix,&info)==FVM_OK&&info.stream.played_frames==2&&f.fifo.count==2);
 return 0;
}
