#include "swf_bitmap_stroke_fixture.hpp"
#include "../ports/thread_port.h"
struct ShapeJob { unsigned seed,done; };
static void worker(void *arg)
{
    ShapeJob *job=(ShapeJob*)arg; FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK); assert(fvm_standard_install(c)==FVM_OK);
    for(unsigned int i=0;i<24;++i) {
        unsigned int kind=0x40+(i+job->seed)%4;
        FVM_Object *root=load(c,bs_tags(bs_shape(kind,80,true,true,10)),1); tick(c,root);
        assert(fvm_gc_collect(c,0)==FVM_OK); bs_pixels(c,root,kind,80,true,true,10);
        assert(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);
        assert(fvm_gc_unpin(c,root)==FVM_OK); assert(fvm_gc_collect(c,0)==FVM_OK); ++job->done;
    }
    fvm_context_destroy(c); fvm_runtime_destroy(rt);
}
int main()
{
    ShapeJob jobs[4]; FVM_PortThread *threads[4];
    for(unsigned int i=0;i<4;++i) { jobs[i].seed=i; jobs[i].done=0; threads[i]=fvm_port_thread_start(worker,jobs+i); assert(threads[i]); }
    for(unsigned int i=0;i<4;++i) { fvm_port_thread_join(threads[i]); assert(jobs[i].done==24); }
    return 0;
}
