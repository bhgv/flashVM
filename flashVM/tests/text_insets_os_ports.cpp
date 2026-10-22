/* Separate runtime/context/font owners perform paragraph inset edits on real OS workers. */
#define FVM_INSETS_FIXTURE_ONLY
#include "textfield_insets_fixture.hpp"
#include "../ports/thread_port.h"
#ifdef FVM_HAS_FONTS
struct InsetJob {unsigned margin,passes;};
static void inset_worker(void *raw)
{
 InsetJob *job=(InsetJob*)raw;FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
 FVM_Object *field=counted_field(c),*format=inset_format(c);inset_bool(c,field,"multiline",true);inset_bool(c,field,"wordWrap",true);inset_number(c,field,"_width",124);inset_string(c,field,"autoSize","left");
 for(unsigned i=0;i<32;++i){inset_number(c,format,"leftMargin",job->margin);inset_apply(c,field,format,"setNewTextFormat");inset_string(c,field,"text","A V A V");assert(counted_metric(c,field,"textWidth")== (job->margin==10?72:60) && counted_metric(c,field,"_height")== (job->margin==10?94:140));assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth);FVM_Path *p=0;assert(fvm_textfield_path(c,field,&p)==FVM_OK && fvm_path_count(p));fvm_path_destroy(p);++job->passes;}
 fvm_context_destroy(c);fvm_runtime_destroy(rt);
}
#endif
int main()
{
#ifdef FVM_HAS_FONTS
 InsetJob jobs[4];FVM_PortThread *threads[4];for(unsigned i=0;i<4;++i){jobs[i].margin=i%2?30:10;jobs[i].passes=0;threads[i]=fvm_port_thread_start(inset_worker,jobs+i);assert(threads[i]);}for(unsigned i=0;i<4;++i){fvm_port_thread_join(threads[i]);assert(jobs[i].passes==32);}
#endif
 return 0;
}
