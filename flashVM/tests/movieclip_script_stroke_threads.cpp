#include "script_stroke_fixture.hpp"
#include "../ports/thread_port.h"
struct ScriptJob { unsigned done; };
static void worker(void *arg) { ScriptJob *job=(ScriptJob*)arg;
 FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0; assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK); ss_context(rt,&c);
 for(unsigned kind=0;kind<4;++kind) for(unsigned cap=0;cap<3;++cap) for(unsigned join=0;join<2;++join) { FVM_Object *clip=ss_clip(c,cap,join,kind); ss_pixels(c,clip,cap,join,kind); assert(fvm_gc_unpin(c,clip)==FVM_OK && fvm_gc_collect(c,0)==FVM_OK); assert(!c->gc_frames && !c->gc_native_depth && !c->script_depth); ++job->done; }
 fvm_context_destroy(c); fvm_runtime_destroy(rt);
}
int main() { ScriptJob jobs[4]; FVM_PortThread *threads[4]; for(unsigned i=0;i<4;++i) { jobs[i].done=0; threads[i]=fvm_port_thread_start(worker,jobs+i); assert(threads[i]); } for(unsigned i=0;i<4;++i) { fvm_port_thread_join(threads[i]); assert(jobs[i].done==24); } return 0; }
