/* Worker-owned contexts select distinct immutable source bytes without sharing mutable font handles. */
#define FVM_FONT_FACES_FIXTURE_ONLY
#include "textfield_font_faces.cpp"
#include "../ports/thread_port.h"
#ifdef FVM_HAS_FONTS
struct FaceJob { unsigned variant; unsigned passes; bool failed; };
static void face_worker(void *raw)
{
 FaceJob *job=(FaceJob*)raw;FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;
 if(fvm_config_init(&cfg)!=FVM_OK || fvm_runtime_create(&cfg,&rt)!=FVM_OK || fvm_context_create(rt,&c)!=FVM_OK){job->failed=true;fvm_runtime_destroy(rt);return;}
 FVM_Object *field=counted_field(c),*format=0;register_styles(c);FVM_Value v,arg,out;assert(fvm_construct(c,"TextFormat",0,0,&format)==FVM_OK && fvm_gc_pin(c,format)==FVM_OK);
 select_style(c,format,job->variant);fvm_value_number(&v,40);assert(fvm_object_set(c,format,"size",&v)==FVM_OK);fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=format;assert(fvm_object_call(c,field,"setNewTextFormat",1,&arg,&out)==FVM_OK);
 assert(fvm_value_string(&v,"right")==FVM_OK && fvm_object_set(c,field,"autoSize",&v)==FVM_OK);
 const double advances[]={24,28,26,30};
 for(unsigned i=0;i<64;++i){assert(fvm_value_string(&v,i%2?"AVA":"AV")==FVM_OK);
  double expected=advances[job->variant]*(i%2?3:2)+4;
  if(fvm_object_set(c,field,"text",&v)!=FVM_OK || counted_metric(c,field,"_width")!=expected || counted_metric(c,field,"_x")!=100-expected || fvm_gc_collect(c,0)!=FVM_OK || c->gc_frames || c->gc_native_depth){job->failed=true;break;}++job->passes;
 }
 fvm_context_destroy(c);fvm_runtime_destroy(rt);
}
#endif
int main()
{
#ifdef FVM_HAS_FONTS
 FaceJob jobs[4];FVM_PortThread *threads[4];for(unsigned i=0;i<4;++i){jobs[i].variant=i;jobs[i].passes=0;jobs[i].failed=false;threads[i]=fvm_port_thread_start(face_worker,jobs+i);assert(threads[i]);}
 for(unsigned i=0;i<4;++i){fvm_port_thread_join(threads[i]);assert(!jobs[i].failed && jobs[i].passes==64);}
#endif
 return 0;
}
