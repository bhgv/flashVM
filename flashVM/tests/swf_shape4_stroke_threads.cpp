#include "swf_shape4_stroke_fixture.hpp"
#include "../ports/thread_port.h"
struct ShapeJob { unsigned seed,done; };
static void worker(void *arg)
{
    ShapeJob *job=(ShapeJob*)arg; FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK); assert(fvm_standard_install(c)==FVM_OK);
    for(unsigned int i=0;i<24;++i) {
        unsigned int spread=(i+job->seed)%3, interpolation=(i/3)%2; int focal=i%2?255:-255;
        FVM_Object *root=load(c,shape4_tags(shape4_stroke(0x13,spread,interpolation,focal,80,15,8,4,true)),1); tick(c,root);
        assert(fvm_gc_collect(c,0)==FVM_OK); stroke4_pixels(c,root,0x13,spread,interpolation,focal/256.0,80,15,true,true);
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
