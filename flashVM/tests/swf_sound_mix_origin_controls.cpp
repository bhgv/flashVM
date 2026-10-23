/* Origin modes preserve retries and copied observations through backend failures and controls. */
#include "swf_sound_mix_origin_fixture.hpp"
int main(int argc,char **argv)
{
 bool exceptions=argc==1;assert(argc==1||(argc==2&&!strcmp(argv[1],"--no-throw")));
 GateFixture f;size_t bytes=0;assert(fvm_swf_sound_mix_queue_storage_required_for_modes(2,2,1,1,&bytes)==FVM_OK);
 assert(fvm_swf_sound_mix_queue_set_origin_tracking(f.mix,1,bytes-1)==FVM_OUT_OF_MEMORY);
 enable_origins(f);f.fifo.reenter=2;short pcm[4]={100,100,200,200};size_t n=0;
 assert(f.adapted.write(f.adapted.userdata,pcm,2)==FVM_OK);f.fifo.count=1;f.fifo.fail_write=1;
 assert(fvm_swf_sound_mix_queue_pump_tail_finite(f.mix,2,4,&n)==FVM_ERROR&&!n);
 FVM_SWFSoundMixQueueOriginInfo before=origins(f);assert(before.base_submitted_frames==2&&before.base_played_frames==1&&!before.tail_submitted_frames);
 assert(fvm_swf_sound_mix_queue_stop_all(f.mix)==FVM_OK);f.fifo.fail_clear=1;
 assert(f.adapted.clear(f.adapted.userdata)==FVM_ERROR);assert(origins(f).base_submitted_frames==2);
 if(exceptions){f.fifo.throw_write=1;assert(fvm_swf_sound_mix_queue_pump_tail_finite(f.mix,2,4,&n)==FVM_ERROR&&!n);f.fifo.throw_write=0;}
 f.fifo.fail_write=0;assert(fvm_swf_sound_mix_queue_pump_tail_finite(f.mix,2,4,&n)==FVM_OK&&n==2);
 assert(origins(f).tail_submitted_frames==2);f.fifo.fail_count=1;FVM_SWFSoundMixQueueOriginInfo info=before;
 assert(fvm_swf_sound_mix_queue_origin_info(f.mix,&info)==FVM_ERROR&&!memcmp(&info,&before,sizeof(info)));
 f.fifo.fail_count=0;f.fifo.count=4;assert(fvm_swf_sound_mix_queue_origin_info(f.mix,&info)==FVM_ERROR);
 f.fifo.count=3;f.fifo.fail_clear=0;assert(f.adapted.clear(f.adapted.userdata)==FVM_OK);
 info=origins(f);assert(!info.base_submitted_frames&&!info.tail_submitted_frames&&!info.base_played_frames&&!info.tail_played_frames);
 assert(fvm_swf_sound_mix_queue_set_origin_tracking(f.mix,1,0)==FVM_OK);return 0;
}
