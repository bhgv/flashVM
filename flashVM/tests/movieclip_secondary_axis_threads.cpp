#include "secondary_axis_fixture.hpp"
#include "../ports/thread_port.h"
struct AxisJob {unsigned done;};
static void worker(void *arg) {AxisJob *job=(AxisJob*)arg;FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK&&fvm_runtime_create(&cfg,&rt)==FVM_OK);ss_context(rt,&c);
for(unsigned n=0;n<24;++n) {int e=n%2?-600:10;FVM_Object *target=sa_target(c),*clip=sa_clip(c,(n/2)%3,n%2,e,n/6);assert(fvm_gc_collect(c,0)==FVM_OK);sa_expected(sa_bounds(c,clip,target,e),(n/2)%3);assert(fvm_gc_unpin(c,clip)==FVM_OK&&fvm_gc_unpin(c,target)==FVM_OK&&fvm_gc_collect(c,0)==FVM_OK);assert(!c->gc_native_depth&&!c->gc_frames);++job->done;}fvm_context_destroy(c);fvm_runtime_destroy(rt);}
int main() {AxisJob jobs[4];FVM_PortThread *threads[4];for(unsigned i=0;i<4;++i){jobs[i].done=0;threads[i]=fvm_port_thread_start(worker,jobs+i);assert(threads[i]);}for(unsigned i=0;i<4;++i){fvm_port_thread_join(threads[i]);assert(jobs[i].done==24);}return 0;}
