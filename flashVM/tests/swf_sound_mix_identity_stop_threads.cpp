/* Four real OS workers independently stop/restart around rejected mixed composition. */
#include "swf_sound_mix_identity_fixture.hpp"
#include "../ports/thread_port.h"
struct StopJob {short first;unsigned done;};
static void worker(void *raw)
{
 StopJob *job=(StopJob*)raw;IdentityFixture f(true,job->first);
 for(unsigned epoch=0;epoch<64;++epoch){f.fifo.total=0;FVM_SWFSoundVoice *v=f.clone(0);assert(fvm_swf_sound_mix_queue_submit_prepared_bounded(f.queue,&v,(size_t)-1)==FVM_OK);size_t n=0;f.fifo.fail=1;assert(fvm_swf_sound_mix_queue_pump_tail(f.queue,2,8,&n)==FVM_ERROR);assert(fvm_swf_sound_mix_queue_stop_all(f.queue)==FVM_OK && !active(f.mixer));v=f.clone(1);assert(fvm_swf_sound_mix_queue_submit_prepared_bounded(f.queue,&v,(size_t)-1)==FVM_OK && !v);f.fifo.fail=0;assert(fvm_swf_sound_mix_queue_pump_tail(f.queue,2,8,&n)==FVM_OK && fvm_swf_sound_mix_queue_pump_tail(f.queue,2,8,&n)==FVM_OK);for(unsigned i=0;i<4;++i)assert(f.fifo.saved[i]==job->first+(short)(i%2));FVM_SWFSoundMixQueueIdentityInfo id;id.struct_size=sizeof(id);assert(fvm_swf_sound_mix_queue_identity_info(f.queue,7,&id)==FVM_OK && id.end_frame==7);assert(fvm_swf_sound_mix_queue_stop_all(f.queue)==FVM_OK && f.fifo.count==4);assert(f.adapted.clear(f.adapted.userdata)==FVM_OK);++job->done;}
}
int main()
{
 StopJob jobs[4];FVM_PortThread *threads[4];for(unsigned i=0;i<4;++i){jobs[i].first=(short)(100+i*100);jobs[i].done=0;threads[i]=fvm_port_thread_start(worker,jobs+i);assert(threads[i]);}for(unsigned i=0;i<4;++i){fvm_port_thread_join(threads[i]);assert(jobs[i].done==64);}return 0;
}
