/* Seed ring wrap, full accepted capacity and exact clock limits without huge PCM or duration runs. */
#include "../src/swf/sound_mix_queue.cpp"
#include "swf_sound_mix_origin_fixture.hpp"
int main()
{
 short pcm[4]={0,0,0,0};size_t n=0;
 {GateFixture f;enable_origins(f);f.mix->origin_head=FVM_SWF_SOUND_MIX_QUEUE_ORIGIN_FRAMES-1;assert(f.adapted.write(f.adapted.userdata,pcm,2)==FVM_OK);assert(fvm_swf_sound_mix_queue_pump_tail_finite(f.mix,2,4,&n)==FVM_OK&&n==2);f.fifo.count=1;FVM_SWFSoundMixQueueOriginInfo info=origins(f);assert(info.base_played_frames==2&&info.tail_played_frames==1&&!info.base_queued_frames&&info.tail_queued_frames==1);}
 {GateFixture f;enable_origins(f);double seed=FVM_SWF_SOUND_OUTPUT_MAX_CLOCK-4;f.mix->clock_submitted=f.mix->clock_played=f.mix->origin_base_submitted=f.mix->origin_base_played=seed;f.mix->origin_head=FVM_SWF_SOUND_MIX_QUEUE_ORIGIN_FRAMES-2;assert(fvm_swf_sound_mix_queue_pump_tail_finite(f.mix,2,4,&n)==FVM_OK&&n==2);assert(f.adapted.write(f.adapted.userdata,pcm,2)==FVM_OK);f.fifo.count=1;FVM_SWFSoundMixQueueOriginInfo info=origins(f);assert(info.base_submitted_frames==seed+2&&info.base_played_frames==seed+1&&info.tail_submitted_frames==2&&info.tail_played_frames==2&&info.base_queued_frames==1);assert(f.adapted.write(f.adapted.userdata,pcm,1)==FVM_UNSUPPORTED);}
 {GateFixture f;enable_origins(f);size_t cap=FVM_SWF_SOUND_MIX_QUEUE_ORIGIN_FRAMES;std::memset(f.mix->origins.begin(),255,f.mix->origins.size());f.mix->clock_submitted=f.mix->origin_base_submitted=(double)cap;f.mix->origin_count=f.fifo.count=cap;assert(f.adapted.write(f.adapted.userdata,pcm,1)==FVM_NOT_FOUND&&!f.fifo.calls&&!f.mix->pending);f.fifo.count=cap-1;assert(f.adapted.write(f.adapted.userdata,pcm,1)==FVM_OK);FVM_SWFSoundMixQueueOriginInfo info=origins(f);assert(info.base_played_frames==1&&info.base_queued_frames==cap&&info.base_submitted_frames==cap+1);}
 return 0;
}
