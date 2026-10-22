/* Gate scheduling, retained retry and cancellation allocate no native storage after preparation. */
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
 GateFixture f;size_t before=allocations;allowance=0;size_t n=0;f.fifo.fail_write=1;assert(fvm_swf_stream_output_pump_mix_until(&f.output,f.mix,0,&n)==FVM_ERROR && !n);f.fifo.fail_write=0;assert(fvm_swf_stream_output_pump_mix_until(&f.output,f.mix,1,&n)==FVM_OK && n==2);assert(fvm_swf_stream_output_pump_mix_until(&f.output,f.mix,1,&n)==FVM_OK && n==2);f.fifo.count=0;assert(fvm_swf_stream_output_pump_mix_until(&f.output,f.mix,0,&n)==FVM_OK && n==2);assert(fvm_swf_stream_output_seek(&f.output,0,0)==FVM_OK);assert(allocations==before);allowance=-1;return 0;
}
