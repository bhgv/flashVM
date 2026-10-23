#include "adaptive_curve_fixture.hpp"
#include "../ports/thread_port.h"
struct CurveJob { unsigned done; };
static void worker(void *arg) { CurveJob *job=(CurveJob*)arg; FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0; assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK); ss_context(rt,&c);
 for(unsigned n=0;n<24;++n) { FVM_Object *clip=dc_clip(c,(n/2)%3,n%2,4,n/6); ss_xy(c,clip,"moveTo",100,100); assert(fvm_gc_collect(c,0)==FVM_OK); dc_expected(rb_bounds(c,clip),(n/2)%3,4); assert(!c->gc_native_depth && !c->gc_frames); assert(fvm_gc_unpin(c,clip)==FVM_OK && fvm_gc_collect(c,0)==FVM_OK); ++job->done; } fvm_context_destroy(c); fvm_runtime_destroy(rt); }
int main() { CurveJob jobs[4]; FVM_PortThread *threads[4]; for(unsigned i=0;i<4;++i) { jobs[i].done=0; threads[i]=fvm_port_thread_start(worker,jobs+i); assert(threads[i]); } for(unsigned i=0;i<4;++i) { fvm_port_thread_join(threads[i]); assert(jobs[i].done==24); } return 0; }
