/* Optional ID table budget/allocation errors preserve mode; tracked hot paths allocate nothing. */
#include <new>
#include <cstdlib>
static long tail_allowance=-1;
static size_t tail_allocations=0;
void *operator new(std::size_t n) throw(std::bad_alloc){++tail_allocations;if(tail_allowance==0)throw std::bad_alloc();if(tail_allowance>0)--tail_allowance;void *p=std::malloc(n?n:1);if(!p)throw std::bad_alloc();return p;}
void operator delete(void *p) throw(){std::free(p);}
void *operator new[](std::size_t n) throw(std::bad_alloc){return ::operator new(n);}
void operator delete[](void *p) throw(){::operator delete(p);}
#include "swf_sound_mix_identity_fixture.hpp"
int main()
{
 IdentityFixture f(false);size_t before=tail_allocations;tail_allowance=0;assert(fvm_swf_sound_mix_queue_set_identity_tracking(f.queue,1,f.adapter_budget-1)==FVM_OUT_OF_MEMORY && tail_allocations==before);assert(fvm_swf_sound_mix_queue_set_identity_tracking(f.queue,1,f.adapter_budget)==FVM_OUT_OF_MEMORY && tail_allocations==before+1);tail_allowance=1;assert(fvm_swf_sound_mix_queue_set_identity_tracking(f.queue,1,f.adapter_budget)==FVM_OK);tail_allowance=-1;FVM_SWFSoundVoice *voice=f.clone(0),*duplicate=f.clone(1);before=tail_allocations;tail_allowance=0;
 assert(fvm_swf_sound_mix_queue_submit_prepared_bounded(f.queue,&voice,(size_t)-1)==FVM_OK && !voice);FVM_SWFSoundMixQueueIdentityInfo info;info.struct_size=sizeof(info);assert(fvm_swf_sound_mix_queue_identity_info(f.queue,7,&info)==FVM_OK && info.end_frame==5);f.fifo.fail=1;size_t n=0;assert(fvm_swf_sound_mix_queue_pump_tail(f.queue,2,8,&n)==FVM_ERROR);assert(fvm_swf_sound_mix_queue_submit_prepared_bounded(f.queue,&duplicate,0)==FVM_OK && !duplicate);f.fifo.fail=0;assert(fvm_swf_sound_mix_queue_pump_tail(f.queue,2,8,&n)==FVM_OK && fvm_swf_sound_mix_queue_pump_tail(f.queue,2,8,&n)==FVM_OK && fvm_swf_sound_mix_queue_pump_tail(f.queue,1,8,&n)==FVM_OK);f.fifo.count=0;int yes=91;assert(fvm_swf_sound_mix_queue_sound_active(f.queue,7,&yes)==FVM_OK && !yes);assert(fvm_swf_sound_mix_queue_stop_id(f.queue,7)==FVM_OK && f.adapted.clear(f.adapted.userdata)==FVM_OK && fvm_swf_sound_mix_queue_set_identity_tracking(f.queue,0,0)==FVM_OK && fvm_swf_sound_mix_queue_clock_end(f.queue)==FVM_OK);assert(tail_allocations==before);tail_allowance=-1;return 0;
}
