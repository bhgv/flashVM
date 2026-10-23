/* Gate scheduling, retained retry and cancellation allocate no native storage after preparation. */
#include <new>
#include <cstdlib>
static long allowance=-1;static size_t allocations=0;
void *operator new(std::size_t n) throw(std::bad_alloc){++allocations;if(!allowance)throw std::bad_alloc();if(allowance>0)--allowance;void *p=std::malloc(n?n:1);if(!p)throw std::bad_alloc();return p;}
void operator delete(void *p) throw(){std::free(p);}
void *operator new[](std::size_t n) throw(std::bad_alloc){return ::operator new(n);}
void operator delete[](void *p) throw(){::operator delete(p);}
#include "swf_sound_mix_origin_fixture.hpp"
int main()
{
 GateFixture f;size_t budget=0;assert(fvm_swf_sound_mix_queue_storage_required_for_modes(2,2,1,1,&budget)==FVM_OK);
 allowance=0;assert(fvm_swf_sound_mix_queue_set_origin_tracking(f.mix,1,budget)==FVM_OUT_OF_MEMORY);allowance=-1;
 FVM_SWFSoundMixQueueOriginInfo info;info.struct_size=sizeof(info);info.base_submitted_frames=91;
 assert(fvm_swf_sound_mix_queue_origin_info(f.mix,&info)==FVM_UNSUPPORTED&&info.base_submitted_frames==91);
 enable_origins(f);size_t before=allocations;allowance=0;size_t n=0;
 for(unsigned i=0;i<20;++i){f.fifo.count=0;f.fifo.fail_write=1;FVM_Result r=fvm_swf_stream_output_pump_mix_finite_until(&f.output,f.mix,1,&n);if(r==FVM_NOT_FOUND)break;assert(r==FVM_ERROR&&!n);f.fifo.fail_write=0;assert(fvm_swf_stream_output_pump_mix_finite_until(&f.output,f.mix,1,&n)==FVM_OK&&n);info=origins(f);FVM_SWFStreamMixInfo paired;paired.struct_size=sizeof(paired);assert(fvm_swf_stream_output_mix_info(&f.output,f.mix,&paired)==FVM_OK&&paired.stream.submitted_frames==paired.origins.base_submitted_frames+paired.origins.tail_submitted_frames);}
 f.fifo.fail_write=0;f.fifo.count=0;info=origins(f);assert(info.base_played_frames==9&&info.tail_played_frames==5);
 assert(fvm_swf_stream_output_seek(&f.output,0,0)==FVM_OK);info=origins(f);assert(!info.base_submitted_frames&&!info.tail_submitted_frames&&allocations==before);allowance=-1;return 0;
}
