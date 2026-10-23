/* Gate scheduling, retained retry and cancellation allocate no native storage after preparation. */
#include <new>
#include <cstdlib>
static long allowance=-1;static size_t allocations=0;
void *operator new(std::size_t n) throw(std::bad_alloc){++allocations;if(!allowance)throw std::bad_alloc();if(allowance>0)--allowance;void *p=std::malloc(n?n:1);if(!p)throw std::bad_alloc();return p;}
void operator delete(void *p) throw(){std::free(p);}
void *operator new[](std::size_t n) throw(std::bad_alloc){return ::operator new(n);}
void operator delete[](void *p) throw(){::operator delete(p);}
#include "swf_sound_output_finite_fixture.hpp"
int main()
{
 FiniteOutput f(8,true);f.start(0,true);size_t n=0;size_t before=allocations;allowance=0;
 for(unsigned i=0;i<3;++i){f.device.queued=0;f.device.failure=1;assert(fvm_swf_sound_output_pump_finite(f.output,&n)==FVM_ERROR&&!n);f.device.failure=0;assert(fvm_swf_sound_output_pump_finite(f.output,&n)==FVM_OK);int pending=0;assert(fvm_swf_sound_output_pending(f.output,&pending)==FVM_OK&&pending);}
 assert(f.device.values==18);f.device.queued=0;int pending=91;assert(fvm_swf_sound_output_pending(f.output,&pending)==FVM_OK&&!pending);
 assert(fvm_swf_sound_output_stop(f.output)==FVM_OK&&allocations==before);allowance=-1;return 0;
}
