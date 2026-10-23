/* Finite horizon, retained retry and cancellation allocate no native storage after preparation. */
#include <new>
#include <cstdlib>
static long allowance=-1;static size_t allocations=0;
void *operator new(std::size_t n) throw(std::bad_alloc){++allocations;if(!allowance)throw std::bad_alloc();if(allowance>0)--allowance;void *p=std::malloc(n?n:1);if(!p)throw std::bad_alloc();return p;}
void operator delete(void *p) throw(){std::free(p);}
void *operator new[](std::size_t n) throw(std::bad_alloc){return ::operator new(n);}
void operator delete[](void *p) throw(){::operator delete(p);}
#include "swf_stream_mix_gate_fixture.hpp"
int main()
{
 GateFixture f;size_t n=0;size_t before=allocations;allowance=0;
 while(f.fifo.total<13){f.fifo.count=0;assert(fvm_swf_stream_output_pump_mix_finite_until(&f.output,f.mix,1,&n)==FVM_OK && n);}
 assert(f.fifo.total==13);f.fifo.count=0;f.fifo.fail_write=1;
 assert(fvm_swf_stream_output_pump_mix_finite_until(&f.output,f.mix,1,&n)==FVM_ERROR && !n);
 FVM_SWFSoundMixQueuePending pending;pending.struct_size=sizeof(pending);
 assert(fvm_swf_sound_mix_queue_pending(f.mix,&pending)==FVM_OK && pending.frames==1);
 assert(fvm_swf_sound_mix_queue_stop_all(f.mix)==FVM_OK);f.fifo.fail_write=0;
 assert(fvm_swf_stream_output_pump_mix_finite_until(&f.output,f.mix,0,&n)==FVM_OK && n==1 && f.fifo.total==14);
 assert(fvm_swf_stream_output_seek(&f.output,0,0)==FVM_OK && allocations==before);allowance=-1;return 0;
}
