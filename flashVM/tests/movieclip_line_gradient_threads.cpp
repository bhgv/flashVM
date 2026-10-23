/* Four OS workers retain isolated fill/stroke graphs through 128 paint and GC epochs. */
#include "line_gradient_fixture.hpp"
#include "../ports/thread_port.h"
struct GradientJob {unsigned seed,done;};
static void gradient_worker(void *raw)
{
    GradientJob *job=(GradientJob*)raw;FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Object *clip=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"worker",1,&clip)==FVM_OK && fvm_gc_pin(c,clip)==FVM_OK);
    for(unsigned epoch=0;epoch<32;++epoch){const unsigned mode=(job->seed+epoch)%3;const bool radial=(epoch%2)!=0;FVM_Value out;assert(fvm_object_call(c,clip,"clear",0,0,&out)==FVM_OK);if((epoch/2)%2){line_gradient_style(c,clip,mode,radial);line_gradient_geometry(c,clip);line_gradient_pixels(c,clip,mode,radial);}else{FVM_Value args[8];spread_arguments(c,args,mode,radial);assert(fvm_object_call(c,clip,"beginGradientFill",8,args,&out)==FVM_OK);geometry(c,clip);spread_pixels(c,clip,mode,radial);}assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);++job->done;}
    fvm_context_destroy(c);fvm_runtime_destroy(rt);
}
int main()
{
    GradientJob jobs[4];FVM_PortThread *threads[4];for(unsigned i=0;i<4;++i){jobs[i].seed=i;jobs[i].done=0;threads[i]=fvm_port_thread_start(gradient_worker,jobs+i);assert(threads[i]);}for(unsigned i=0;i<4;++i){fvm_port_thread_join(threads[i]);assert(jobs[i].done==32);}return 0;
}
